% ------------------------------------------------------------------------------ -
% 
% Copyright (c) 2018 Koninklijke Philips N.V.
% 
% Authors : Bart Kroon, Bart Sonneveldt
% Contact : bart.kroon@philips.com
% 
% MATLAB script to convert 3DoF+ CfTM style metadata JSON's to the format 
% used by RVS, which is based on the SVS configuration file format.
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
