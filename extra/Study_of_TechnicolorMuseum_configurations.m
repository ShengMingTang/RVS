% ------------------------------------------------------------------------------ -
% 
% Copyright (c) 2018 Koninklijke Philips N.V.
% 
% Authors : Bart Kroon
% Contact : bart.kroon@philips.com
% 
% SVS 3DoF+
% Study of the angle system used in TechnicolorMuseum
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

file = fopen('TechnicolorMuseum/metadata.json');
text = fread(file, '*char');
metadata = jsondecode(text);

position = bsxfun(@minus, [metadata.cameras.Position]', metadata.BoundingBox_center');
rotation = [metadata.cameras.Rotation]';

plot3([zeros(24, 1) 1e3*position(:,1)]', [zeros(24, 1) 1e3*position(:,2)]', [zeros(24, 1) 1e3*position(:,3)]');
hold on;
scatter3(1e3*position(:,1), 1e3*position(:,2), 1e3*position(:,3), 'k.');
daspect([1 1 1]);
grid;
xlabel('x (forward) [mm]');
ylabel('y (left) [mm]');
zlabel('z (up) [mm]');

for n = 1:24
    R_yaw   = [ cosd(rotation(n,1)) -sind(rotation(n,1))  0
                sind(rotation(n,1))  cosd(rotation(n,1))  0
                0                    0                    1                   ];
    R_pitch = [ cosd(rotation(n,2))  0                    sind(rotation(n,2))
                0                    1                    0
               -sind(rotation(n,2))  0                    cosd(rotation(n,2)) ];
    R_roll  = [ 1                    0                    0
                0                    cosd(rotation(n,3)) -sind(rotation(n,3))
                0                    sind(rotation(n,3))  cosd(rotation(n,3)) ];

    R = R_yaw * R_pitch * R_roll;
    x_dir = [1 0 0]';
    v = R*x_dir;
    quiver3(1e3*position(n,1), 1e3*position(n,2), 1e3*position(n,3), v(1), v(2), v(3), 100, 'k');
end

hold off;
fig = gcf;
fig.PaperUnits = 'centimeters';
fig.PaperPosition = [0 0 30 30];
saveas(fig, [mfilename '.png']);