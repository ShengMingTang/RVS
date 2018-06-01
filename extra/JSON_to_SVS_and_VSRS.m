% ------------------------------------------------------------------------------ -
% 
% Copyright ? 2018 Koninklijke Philips N.V.
% 
% Authors : Bart Kroon, Bart Sonneveldt
% Contact : bart.kroon@philips.com
% 
% MATLAB script to convert 3DoF+ CfTM style metadata JSON's to the format 
% used by the SVS extension (feature/3DoFp) and also something that 
% somewhat resembles VSRS.
% 
% Permission is hereby granted, free of charge, to the members of the Moving Picture
% Experts Group(MPEG) obtaining a copy of this software and associated documentation
% files(the "Software"), to use the Software exclusively within the framework of the
% MPEG - I(immersive) and MPEG - I Visual activities, for the sole purpose of
% developing the MPEG - I standard.This permission explicitly excludes the rights
% to publish, distribute, sublicense, sell, embed into a product or a service and / or
% otherwise commercially exploit copies of the Software without the written consent
% of the owner(Koninklijke Philips N.V.).
% 
% This permission is provided subject to the following conditions :
% The above copyright notice and this permission notice shall be included in all
% copies, substantial portions or derivative works of the Software.
% 
% ------------------------------------------------------------------------------ -


function JSON_to_SVS_and_VSRS(...
    input_view_metadata_path, ...
    output_view_metadata_path, ...
    camparams_path, ...
    vsrs_config_pathfmt, ...
    svs_config_path, ...
    depth_range_path, ...
    texture_bitdepth, ...
    depth_bitdepth, ...
    texture_pathfmt, ...
    depth_pathfmt, ...
    output_pathfmt, ...
    input_view_indices, ...
    output_view_indices, ...
    max_frames)

% Naming convention of CTC 10-bit streams

% Load first JSON with input view positions
file = fopen(input_view_metadata_path);
input_view_metadata = jsondecode(fread(file, '*char')');
fclose(file);

% Load second JSON with output view positions (potentially the same file)s
file = fopen(output_view_metadata_path);
output_view_metadata = jsondecode(fread(file, '*char')');
fclose(file);

% Fix format differences
if isfield(output_view_metadata, 'metadata')
    output_view_metadata = output_view_metadata.metadata;
    output_view_metadata.cameras = rmfield(output_view_metadata.cameras, 'closest_view');
end

% Shorthands
Ci = input_view_metadata.cameras;
Co = output_view_metadata.cameras;
Ni = length(Ci);
No = length(Co);

% Verify: input views are equal resolution
resolution = Ci(1).Resolution;
for m = 2:Ni
    if ~isequal(resolution, Ci(m).Resolution)
        error('Input views should have equal resolution');
    end
end

% Verify: input (source) camera names are unique
for m = 1:Ni
    for n = m + 1:Ni
        if isequal(Ci(m).Name, Ci(n).Name)
            error('Input camera names should be unique');
        end
    end
end

% Verify: output (source/intermediate) camera names are unique
for m = 1:No
    for n = m + 1:No
        if isequal(Co(m).Name, Co(n).Name)
            error('Output camera names should be unique');
        end
    end
end

% Verify: cameras with equal name are equal
for m = 1:Ni
    for n = 1:No
        if isequal(Ci(m).Name, Co(n).Name) && ~isequal(Ci(m), Co(n))
            error('Cameras with equal names should be equal');
        end
    end
end

% Combine the camera list
C = Ci;
for m = 1:No
    have = false;
    for n = 1:Ni
        if isequal(Co(m).Name, Ci(n).Name)
            have = true;
            break;
        end
    end
    if ~have
        C(end + 1,1) = Co(m); %#ok<AGROW>
    end
end

% Affine transformation: x --> R^T (x - t)
% Whereby "x" is OMAF Referential: x forward, y left, z up,
% But "t" and "R" are stored in VSRS system: x right, y down, z forward
% Define P such that x == P x_VSRS (same as Parameters.cpp)
% x --> P R_VSRS^T P^T (x - P t_VSRS)

%    right down forward
P = [  0     0     1     % forward
      -1     0     0     % left
       0    -1     0 ];  % up

% Write out camera parameters
file = fopen(camparams_path, 'w');
for n = 1:length(C)
    fprintf(file, '%s\n', C(n).Name);
    fake = [2 0 1; 0 2 1; 0 0 1];
    fprintf(file, '%g %g %g\n', fake');
    fprintf(file, '0\n');
    fprintf(file, '0\n');
        
    R_VSRS = P' * EulerAnglesToRotationMatrix(C(n).Rotation) * P;
    t_VSRS = P' * C(n).Position;
    
    M = [R_VSRS t_VSRS]; 
    fprintf(file, '%g %g %g %g\n', M');
    fprintf(file, '\n');
end
fclose(file);

% String formatting
input_texture_path = cell(Ni, 1);
input_depth_path = cell(Ni, 1);

for k = 1:Ni
    Rmin_s = sprintf('%.6f', Ci(k).Rmin);
    Rmin_s = strrep(Rmin_s, '.', '_');
    while Rmin_s(end) == '0' && Rmin_s(end - 1) ~= '_', Rmin_s = Rmin_s(1:end - 1); end
    Rmax_s = sprintf('%.6f', Ci(k).Rmax);
    Rmax_s = strrep(Rmax_s, '.', '_');
    while Rmax_s(end) == '0' && Rmax_s(end - 1) ~= '_', Rmax_s = Rmax_s(1:end - 1); end
    if Ci(k).Rmax >= 1000, Ci(k).Rmax = 1e6; end
    input_texture_path{k} = sprintf(texture_pathfmt, Ci(k).Name, Ci(k).Resolution);
    input_depth_path{k} = sprintf(depth_pathfmt, Ci(k).Name, Ci(k).Resolution, Rmin_s, Rmax_s);
end

%
% VSRS
%

if isequal('format', 'final')
    
% Generate configuration files per output view and frame
for m = 1:length(output_view_indices)
    oi = 1 + output_view_indices(m);

    vsrs_config_path = sprintf(vsrs_config_pathfmt, Co(oi).Name);
    vsrs_file = fopen(vsrs_config_path, 'w');

    fprintf(vsrs_file, 'DepthType 1\n');
    fprintf(vsrs_file, 'SourceWidth %d\n', resolution(1));
    fprintf(vsrs_file, 'SourceHeight %d\n', resolution(2));
    fprintf(vsrs_file, 'StartFrame 0\n');
    fprintf(vsrs_file, 'TotalNumberOfFrames %d\n', min(max_frames, input_view_metadata.Frames_number));
    fprintf(vsrs_file, 'NumberOfInputs %d\n', length(input_view_indices));
    fprintf(vsrs_file, '\n');
   
    % NOTE: We need to decide on a naming system
    %       This is a suggestion but it is not backwards compatible
    for k = 1:length(input_view_indices)
        ii = 1 + input_view_indices(k);
        fprintf(vsrs_file, 'Input%dNearestDepthValue %g\n', k - 1, Ci(ii).Rmin);
        fprintf(vsrs_file, 'Input%dFarthestDepthValue %g\n', k - 1, Ci(ii).Rmax);
        fprintf(vsrs_file, 'Input%dImageName %s\n', k - 1, input_texture_path{ii});
        fprintf(vsrs_file, 'Input%dDepthMapName %s\n', k - 1, input_depth_path{ii});
        fprintf(vsrs_file, '\n');        
    end
    
    fprintf(vsrs_file, 'CameraParameterFile %s\n', camparams_path);
    fprintf(vsrs_file, 'VirtualCameraName %s\n', Co(oi).Name);
    output_texture_path = sprintf(output_pathfmt, Co(oi).Name, resolution);
    fprintf(vsrs_file, 'OutputVirtualViewImageName %s\n', output_texture_path);
    fprintf(vsrs_file, 'ColorSpace 0\n');
    fprintf(vsrs_file, 'Precision 4\n');
    fprintf(vsrs_file, 'Filter 0\n');
    fprintf(vsrs_file, 'BoundaryNoiseRemoval 0\n');
    fprintf(vsrs_file, 'SynthesisMode 2 \n');
    fprintf(vsrs_file, 'ViewBlending 0\n');
    fclose(vsrs_file);
end

end

%
% SVS
%

svs_file = fopen(svs_config_path, 'w');

fprintf(svs_file, 'SVSFile 1\n\n');
fprintf(svs_file, 'InputCameraParameterFile\n%s\n\n', camparams_path);   
fprintf(svs_file, 'InputProjectionType Equirectangular\n\n');
fprintf(svs_file, 'InputCameraNumber %d\n\n', length(input_view_indices));
fprintf(svs_file, 'ViewImagesNames\n%s\n', sprintf('%s\n', input_texture_path{1 + input_view_indices}));
fprintf(svs_file, 'DepthMapsNames\n%s\n', sprintf('%s\n', input_depth_path{1 + input_view_indices}));
fprintf(svs_file, 'ZValues\n%s\n\n', depth_range_path);
fprintf(svs_file, 'CamerasNames\n%s\n', sprintf('%s\n', Ci(1 + input_view_indices).Name));
fprintf(svs_file, 'VirtualCameraParamaterFile\n%s\n\n', camparams_path);
fprintf(svs_file, 'VirtualProjectionType Equirectangular\n\n');
fprintf(svs_file, 'VirtualCameraNumber %d\n\n', length(output_view_indices));
fprintf(svs_file, 'VirtualCamerasNames\n%s\n', sprintf('%s\n', Co(1 + output_view_indices).Name));
fprintf(svs_file, 'OuputDir\n./\n\n');
fprintf(svs_file, 'OutputFiles\n');
for n = 1:length(output_view_indices)
    oi = 1 + output_view_indices(n);
    fprintf(svs_file, '%s\n', sprintf(output_pathfmt, Co(oi).Name, resolution));
end
fprintf(svs_file, '\nExtension\nyuv\n\n');
fprintf(svs_file, 'BitDepthColor %d\n\n', texture_bitdepth);
fprintf(svs_file, 'BitDepthDepth %d\n\n', depth_bitdepth);
fprintf(svs_file, 'StartFrame 0\n\n');
fprintf(svs_file, 'NumberOfFrames %d\n\n', min(max_frames, input_view_metadata.Frames_number));
fprintf(svs_file, 'Width %d\n\n', resolution(1));
fprintf(svs_file, 'Height %d\n\n', resolution(2));
fprintf(svs_file, 'Precision 2.0\n\n');
fprintf(svs_file, 'ColorSpace YUV\n\n');
fprintf(svs_file, 'ViewSynthesisMethod Triangles\n\n');
fprintf(svs_file, 'BlendingMethod Simple\n\n');
fprintf(svs_file, 'BlendingFactor 5.0\n\n'); % suggested by Sarah
fprintf(svs_file, 'SensorSize 1.0\n\n');
fclose(svs_file);

%
% Depth range file
%

depth_range_file = fopen(depth_range_path, 'w');
for k = 1:Ni
    fprintf(depth_range_file, '%s\n', input_depth_path{k});
    fprintf(depth_range_file, 'NearestDepthValue %f\n', Ci(k).Rmin);
    fprintf(depth_range_file, 'FarthestDepthValue %f\n\n', Ci(k).Rmax);
end
fclose(depth_range_file);