% ------------------------------------------------------------------------------ -
% 
% Copyright (c) 2018 Koninklijke Philips N.V.
% 
% Authors : Bart Sonneveldt, Bart Kroon
% Contact : bart.kroon@philips.com
% 
% SVS 3DoF+
% For the purpose of the 3DoF+ Investigation, SVS is extended to match with the
% description of the Reference View Synthesizer(RVS) of the 3DoF+ part of the CTC.
% This includes support for unprojecting and projecting ERP images, reading and
% writing of 10 - bit YUV 4:2 : 0 texture and depth, parsing of JSON files according to
% the 3DoF + CfTM, and unit / integration tests.
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

function R = EulerAnglesToRotationMatrix(euler)
    z = euler(1);
    y = euler(2);
    x = euler(3);

    Rx = RotationMatrixFromRotationAroundX(x);
    Ry = RotationMatrixFromRotationAroundY(y);
    Rz = RotationMatrixFromRotationAroundZ(z);

    R = Rz * Ry * Rx;
end

function R = RotationMatrixFromRotationAroundX( rx )
    R = [1 0 0
        0 cos(rx) -sin(rx)
        0 sin(rx)  cos(rx)];
end

function R = RotationMatrixFromRotationAroundY( ry )
    R = [  cos(ry) 0 sin(ry)
           0       1 0
          -sin(ry) 0 cos(ry) ];
end

function R = RotationMatrixFromRotationAroundZ( rz )
    R = [ cos(rz) -sin(rz) 0
          sin(rz)  cos(rz) 0
          0        0       1];
end