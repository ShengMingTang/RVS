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


% Please notice that SVS is kept in the filenames to indicate the type of
% configuration file (as opposed to VSRS). RVS is capable of handling SVS
% and VSRS-style configuration files.

max_frames = 1; % Put this to infinity to disable this functionality
sequences = {...
	'ClassroomVideo'
	'TechnicolorMuseum'
	'TechnicolorHijack'};
test_classes = {...
	{'A1', 'A2', 'A3'}
	{'B1', 'B2', 'B3'}
	{'C1', 'C2', 'C3'}};
source_views = {...
	0:14
	0:23
	0:9};
anchor_coded_views = {...
	{0:14, [0, 7:14], 0}
	{0:23, [0 1 4 8 11 12 13 17], [0 2 13 17 19]}
	{0:9, [1 4 5 8 9], [1 4]}};
intermediate_views = {...
	0:7
	0:19
	0:14};
metadata_files = {...
	'ClassroomVideo.json', 'ClassroomVideo-IntermediateViews.json'
	'metadata.json', 'TechnicolorMuseum-IntermediateViews.json'
	'metadata.json', 'TechnicolorHijack-IntermediateViews.json'};
virtual_size = {...
	[4096 2048]
	[2048 2048]
	[4096 4096]};

% Directory structure
for i = 1:length(sequences)
	if ~exist(sprintf('config_files/%s', sequences{i}), 'dir')
		mkdir(sprintf('config_files/%s', sequences{i}));
	end
	if ~exist(sprintf('output/%s', sequences{i}), 'dir')
		mkdir(sprintf('output/%s', sequences{i}));
	end
	if ~exist(sprintf('output/%s/ref', sequences{i}), 'dir')
		mkdir(sprintf('output/%s/ref', sequences{i}));
	end
	for j = 1:length(test_classes{i})
		if ~exist(sprintf('output/%s/%s', sequences{i}, test_classes{i}{j}), 'dir')
			mkdir(sprintf('output/%s/%s', sequences{i}, test_classes{i}{j}));
		end
	end
	if ~exist(sprintf('masked_output/%s', sequences{i}), 'dir')
		mkdir(sprintf('masked_output/%s', sequences{i}));
	end
	if ~exist(sprintf('masked_output/%s/ref', sequences{i}), 'dir')
		mkdir(sprintf('masked_output/%s/ref', sequences{i}));
	end
	for j = 1:length(test_classes{i})
		if ~exist(sprintf('masked_output/%s/%s', sequences{i}, test_classes{i}{j}), 'dir')
			mkdir(sprintf('masked_output/%s/%s', sequences{i}, test_classes{i}{j}));
		end
	end
end

% Source views
for i = 1:length(sequences)
	for j = 1:length(test_classes{i})
		for k = 1:length(source_views{i})
			JSON_to_SVS(...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 1}), ...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 1}), ...
				sprintf('config_files/%s/%s-sv-SVS-v%d.txt', sequences{i}, test_classes{i}{j}, source_views{i}(k)), ...
				10, 10, ...
				sprintf('%s/%%s_%%d_%%d_420_10b.yuv', sequences{i}), ...
				sprintf('%s/%%s_%%d_%%d_%%s_%%s_420_10b.yuv', sequences{i}), ...
				sprintf('output/%s/%s/%s_%%s_%%d_%%d_420_10b.yuv', sequences{i}, test_classes{i}{j}, test_classes{i}{j}), ...
				sprintf('masked_output/%s/%s/%s_%%s_%%d_%%d_420_10b_masked.yuv', sequences{i}, test_classes{i}{j}, test_classes{i}{j}), ...
				anchor_coded_views{i}{j}, ...
				source_views{i}(k), ...
				max_frames, ...
				virtual_size{i});
		end
	end
end

% Intermediate-view reference
for i = 1:length(sequences)
	for j = 1:length(test_classes{i})
		for k = 1:length(intermediate_views{i})
			JSON_to_SVS(...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 1}), ...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 2}), ...
				sprintf('config_files/%s/ref-SVS-i%d.txt', sequences{i}, intermediate_views{i}(k)), ...
				10, 16, ...
				sprintf('%s/%%s_%%d_%%d_420_10b.yuv', sequences{i}), ...
				sprintf('%s/%%s_%%d_%%d_%%s_%%s_420_16b.yuv', sequences{i}), ...
				sprintf('output/%s/ref/ref_%%s_%%d_%%d_420_10b.yuv', sequences{i}), ...
				sprintf('masked_output/%s/ref/ref_%%s_%%d_%%d_420_10b_masked.yuv', sequences{i}), ...
				source_views{i}, ...
				intermediate_views{i}(k), ...
				max_frames, ...
				virtual_size{i});
		end
	end
end

% Intermediate views
for i = 1:length(sequences)
	for j = 1:length(test_classes{i})
		for k = 1:length(intermediate_views{i})
			JSON_to_SVS(...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 1}), ...
				sprintf('%s/%s', sequences{i}, metadata_files{i, 2}), ...
				sprintf('config_files/%s/%s-iv-SVS-i%d.txt', sequences{i}, test_classes{i}{j}, intermediate_views{i}(k)), ...
				10, 10, ...
				sprintf('%s/%%s_%%d_%%d_420_10b.yuv', sequences{i}), ...
				sprintf('%s/%%s_%%d_%%d_%%s_%%s_420_10b.yuv', sequences{i}), ...
				sprintf('output/%s/%s/%s_%%s_%%d_%%d_420_10b.yuv', sequences{i}, test_classes{i}{j}, test_classes{i}{j}), ...
				sprintf('masked_output/%s/%s/%s_%%s_%%d_%%d_420_10b_masked.yuv', sequences{i}, test_classes{i}{j}, test_classes{i}{j}), ...
				anchor_coded_views{i}{j}, ...
				intermediate_views{i}(k), ...
				max_frames, ...
				virtual_size{i});
		end	
	end
end
