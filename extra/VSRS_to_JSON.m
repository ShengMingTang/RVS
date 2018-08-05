% Tip: Use Notepad++ with JSTool plug-in to format the resulting file
function VSRS_to_JSON(in_filepath, out_filepath, scale, metadata)
    if ~exist('metadata', 'var')
        % Either specify all metadata or start with these default values 
        % and edit the resulting JSON file.
        metadata = struct(...
            'Version', '2.0', ...
            'Content_name', 'ULB_Unicorn', ...
            'BoundingBox_center', [0 0 0], ...
            'Fps', 1, ...
            'Frames_number', 1, ...
            'Informative', struct(...
                'Converted_by', 'VSRS_to_JSON.m', ...
                'Original_units', 'mm', ...
                'New_units', 'm'), ... % Because depth has to be < 1000
            'cameras', struct(...
                'Resolution', [1920 1080], ...
                'Depth_range', [0.5 2], ... % meters
                'BitDepthColor', 8, ...
                'BitDepthDepth', 16));
            
        scale = 1e-3; % Convert from millimeter to meter
    end
        
    file = fopen(in_filepath);
    cameras = [];
    
    permute = [ 0   0  1
               -1   0  0
                0  -1  0];
    
    while true
        Name = fgetl(file);
        if isequal(Name, -1) || isequal(Name, ''), break; end
        
        Intrinsics = fscanf(file, '%f', [3 3])';
        fscanf(file, '%f', [2 1]);
        Extrinsics = fscanf(file, '%f', [4 3])';
        fgetl(file);
        
        Focal = Intrinsics([1 5]);
        Principle_point = Intrinsics(1:2, 3);
        RotationMatrix = (permute * Extrinsics(:,1:3) * permute')';
        Position = scale * permute * Extrinsics(:,4);
        Rotation = RotationMatrixToEulerAngles(RotationMatrix);
                
        p = struct(...
            'Name', Name, ...
            'Position', Position, ...
            'Rotation', Rotation, ...
            'Depthmap', 1, ...
            'Background', 0, ...
            'Depth_range', metadata.cameras.Depth_range, ...
            'Resolution', metadata.cameras.Resolution, ...
            'Projection', 'Perspective', ...
            'Focal', Focal, ...
            'Principle_point', Principle_point, ...
            'BitDepthColor', metadata.cameras.BitDepthColor, ...
            'BitDepthDepth', metadata.cameras.BitDepthDepth, ...
            'ColorSpace', 'YUV420', ...
            'DepthColorSpace', 'YUV420');
        
        if isempty(cameras)
            cameras = p;
        else
            cameras(end + 1) = p; %#ok<AGROW>
        end

        line = fgetl(file);
        if isequal(line, -5), break; end
        if ~isequal(line, ''), error('Format error'); end
    end
    fclose(file);
    
    metadata.cameras = cameras;
    text = jsonencode(metadata);
    file = fopen(out_filepath, 'w');
    fwrite(file, text, '*char');
    fclose(file);
end

function R = rotz(yaw)
    R = [cos(yaw)  -sin(yaw) 0
         sin(yaw)   cos(yaw) 0
         0          0        1];
end

function R = roty(pitch)
    R = [ cos(pitch) 0 sin(pitch)
          0          1 0
         -sin(pitch) 0 cos(pitch) ];
end

function R = rotx(roll)
    R = [ 1      0     0
          0  cos(roll) -sin(roll)
          0  sin(roll)  cos(roll) ];
end

function rotation = RotationMatrixToEulerAngles(R)
    eps = 1e-7;
    if max(abs(R(1:2, 1))) < eps % Gimbal lock check
        yaw = atan2(R(2, 3), R(1, 3));
        if R(3, 1) < 0
            pitch = pi/2;
        else
            pitch = -pi/2;
        end
        roll = 0;
    else
        yaw = atan2(R(2, 1), R(1, 1));        
        if abs(R(1, 1)) < eps
            pitch = atan2(-R(3, 1), R(2, 1) / sin(yaw) );
        else
            pitch = atan2(-R(3, 1), R(1, 1) / cos(yaw) );
        end        
        roll = atan2(R(3, 2), R(3, 3));
    end
    rotation = rad2deg([yaw pitch roll]);
    
    % Check
    % TODO: remove
    recon = rotz(yaw) * roty(pitch) * rotx(roll);
    if norm(R * recon' - eye(3)) > 1e-6
        error('Bug in RotationMatrixToEulerAngles');
    end
end
