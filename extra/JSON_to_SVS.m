% The copyright in this software is being made available under the BSD
% License, included below. This software may be subject to other third party
% and contributor rights, including patent rights, and no such rights are
% granted under this license.
%
% Copyright (c) 2010-2018, ITU/ISO/IEC
% All rights reserved.
%
% Redistribution and use in source and binary forms, with or without
% modification, are permitted provided that the following conditions are met:
%
%  * Redistributions of source code must retain the above copyright notice,
%    this list of conditions and the following disclaimer.
%  * Redistributions in binary form must reproduce the above copyright notice,
%    this list of conditions and the following disclaimer in the documentation
%    and/or other materials provided with the distribution.
%  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
%    be used to endorse or promote products derived from this software without
%    specific prior written permission.
%
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
% ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
% BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
% CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
% SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
% INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
% CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
% ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
% THE POSSIBILITY OF SUCH DAMAGE.


% Original authors:
% 
% Universite Libre de Bruxelles, Brussels, Belgium:
% 	Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
% 	Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
% 	Arnaud Schenkel, arnaud.schenkel@ulb.ac.be
% 
% Koninklijke Philips N.V., Eindhoven, The Netherlands:
% 	Bart Kroon, bart.kroon@philips.com
% 	Bart Sonneveldt, bart.sonneveldt@philips.com


function JSON_to_SVS(...
    input_view_metadata_path, ...
    output_view_metadata_path, ...
    svs_config_path, ...
    texture_bitdepth, ...
    depth_bitdepth, ...
    texture_pathfmt, ...
    depth_pathfmt, ...
    output_pathfmt, ...
    masked_output_pathfmt, ...
    input_view_indices, ...
    output_view_indices, ...
    max_frames, ...
    virtual_size)

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
% SVS-derived configuration file format
%

svs_file = fopen(svs_config_path, 'w');

fprintf(svs_file, 'SVSFile 1\n\n');
fprintf(svs_file, 'InputCameraParameterFile\n%s\n\n', input_view_metadata_path);   
fprintf(svs_file, 'InputProjectionType Equirectangular\n\n');
fprintf(svs_file, 'InputCameraNumber %d\n\n', length(input_view_indices));
fprintf(svs_file, 'ViewImagesNames\n%s\n', sprintf('%s\n', input_texture_path{1 + input_view_indices}));
fprintf(svs_file, 'DepthMapsNames\n%s\n', sprintf('%s\n', input_depth_path{1 + input_view_indices}));
fprintf(svs_file, 'CamerasNames\n%s\n', sprintf('%s\n', Ci(1 + input_view_indices).Name));
fprintf(svs_file, 'VirtualCameraParamaterFile\n%s\n\n', output_view_metadata_path);
fprintf(svs_file, 'VirtualProjectionType Equirectangular\n\n');
fprintf(svs_file, 'VirtualCameraNumber %d\n\n', length(output_view_indices));
fprintf(svs_file, 'VirtualCamerasNames\n%s\n', sprintf('%s\n', Co(1 + output_view_indices).Name));
fprintf(svs_file, 'OuputDir\n./\n\n');
fprintf(svs_file, 'OutputFiles\n');
for n = 1:length(output_view_indices)
    oi = 1 + output_view_indices(n);
    fprintf(svs_file, '%s\n', sprintf(output_pathfmt, Co(oi).Name, resolution));
end
fprintf(svs_file, '\nMaskedOutputFiles\n');
for n = 1:length(output_view_indices)
    oi = 1 + output_view_indices(n);
    fprintf(svs_file, '%s\n', sprintf(masked_output_pathfmt, Co(oi).Name, resolution));
end
fprintf(svs_file, '\nExtension\nyuv\n\n');
fprintf(svs_file, 'BitDepthColor %d\n\n', texture_bitdepth);
fprintf(svs_file, 'BitDepthDepth %d\n\n', depth_bitdepth);
fprintf(svs_file, 'StartFrame 0\n\n');
fprintf(svs_file, 'NumberOfFrames %d\n\n', min(max_frames, input_view_metadata.Frames_number));
fprintf(svs_file, 'Width %d\n\n', resolution(1));
fprintf(svs_file, 'Height %d\n\n', resolution(2));
fprintf(svs_file, 'VirtualWidth %d\n\n', virtual_size(1));
fprintf(svs_file, 'VirtualHeight %d\n\n', virtual_size(2));
fprintf(svs_file, 'Precision 1.0\n\n');
fprintf(svs_file, 'ColorSpace YUV\n\n');
fprintf(svs_file, 'ViewSynthesisMethod Triangles\n\n');
fprintf(svs_file, 'BlendingMethod Simple\n\n');
fprintf(svs_file, 'BlendingFactor 5.0\n\n'); % suggested by Sarah
fprintf(svs_file, 'SensorSize 1.0\n\n');
fprintf(svs_file, 'ValidityThreshold 5000.0\n\n'); % suggested by Sarah
fclose(svs_file);
