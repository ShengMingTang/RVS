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
    R_BartS = EulerAnglesToRotationMatrix(degtorad(rotation(n,:)));
    fprintf('n=%d, error is %s\n', n, norm(R*R_BartS' - eye(3)));
    x_dir = [1 0 0]';
    v = R*x_dir;
    quiver3(1e3*position(n,1), 1e3*position(n,2), 1e3*position(n,3), v(1), v(2), v(3), 100, 'k');
end

hold off;
fig = gcf;
fig.PaperUnits = 'centimeters';
fig.PaperPosition = [0 0 30 30];
saveas(fig, [mfilename '.png']);