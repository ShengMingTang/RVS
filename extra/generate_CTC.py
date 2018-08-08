# The copyright in this software is being made available under the BSD
# License, included below. This software may be subject to other third party
# and contributor rights, including patent rights, and no such rights are
# granted under this license.
#
# Copyright (c) 2010-2018, ITU/ISO/IEC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
#    be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


# Original authors:
# 
# Universite Libre de Bruxelles, Brussels, Belgium:
# 	Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
# 	Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
# 	Arnaud Schenkel, arnaud.schenkel@ulb.ac.be
# 
# Koninklijke Philips N.i., Eindhoven, The Netherlands:
# 	Bart Kroon, bart.kroon@philips.com
# 	Bart Sonneveldt, bart.sonneveldt@philips.com

import json
import os

sequences = [
	'ClassroomVideo',
	'TechnicolorMuseum',
	'TechnicolorHijack']

test_classes = [
	['A1', 'A2', 'A3'],
	['B1', 'B2', 'B3'],
	['C1', 'C2', 'C3']]

anchor_coded_views = [[
		['v0', 'v1', 'v2', 'v3', 'v4', 'v5', 'v6', 'v7', 'v8', 'v9', 'v10', 'v11', 'v12', 'v13', 'v14'],
		['v0', 'v7', 'v8', 'v9', 'v10', 'v11', 'v12', 'v13', 'v14'],
		['v0']
	], [
		['v0', 'v1', 'v2', 'v3', 'v4', 'v5', 'v6', 'v7', 'v8', 'v9', 'v10', 'v11', 'v12', 'v13', 'v14', 'v15', 'v16', 'v17', 'v18', 'v19', 'v20', 'v21', 'v22', 'v23'],
		['v0', 'v1', 'v1', 'v4', 'v8', 'v11', 'v12', 'v13', 'v17'],
		['v0', 'v2', 'v13', 'v17', 'v19']
	], [
		['v0', 'v1', 'v2', 'v3', 'v4', 'v5', 'v6', 'v7', 'v8', 'v9'],
		['v1', 'v4', 'v5', 'v8', 'v9'],
		['v1', 'v4']
	]]

source_views = list(map(lambda a: a[0], anchor_coded_views))

intermediate_views = [
		['i0', 'i1', 'i2', 'i3', 'i4', 'i5', 'i6', 'i7'],
		['i0', 'i1', 'i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8', 'i9', 'i10', 'i11', 'i12', 'i13', 'i14', 'i15', 'i16', 'i17', 'i18', 'i19'],
		['i0', 'i1', 'i2', 'i3', 'i4', 'i5', 'i6', 'i7', 'i8', 'i9', 'i10', 'i11', 'i12', 'i13', 'i14']]

def depth_format(value):
	text = '{:g}'.format(value)
	if text.find('.') < 0:
		text = text + '.0'
	return text.replace('.', '_')

def generate_configuration(sequence, test_class, input_cameras, virtual_camera, input_overrides):
	# Load sequence metadata file
	metadata_filepath = 'config_files/{}.json'.format(sequence)
	with open(metadata_filepath) as file:
		metadata = json.load(file)

	# Build a camera dictionary
	cameras = {}
	for camera in metadata['cameras']:
		for key, value in enumerate(input_overrides):
			camera[key] = value
		cameras[camera['Name']] = camera

	# Input filepaths
	view_image_names = []
	depth_map_names = []
	for name in input_cameras:
		camera = cameras[name]
		w, h = camera['Resolution']
		near, far = camera['Depth_range']
		near = depth_format(near)
		far = depth_format(far)
		view_image_names.append('{}/{}_{}_{}_420_{}b.yuv'.format(sequence, name, w, h, camera['BitDepthColor']))
		depth_map_names.append('{}/{}_{}_{}_{}_{}_420_{}b.yuv'.format(sequence, name, w, h, near, far, camera['BitDepthDepth']))

	# Output file paths
	output_files = []
	for name in [virtual_camera]:
		camera = cameras[virtual_camera]
		w, h = camera['Resolution']
		output_files.append('{}/{}_{}_{}_{}_420_{}b.yuv'.format(sequence, test_class, name, w, h, camera['BitDepthColor']))

	# Generate the configuration
	configuration = {
		'Version': '2.0',
		'InputCameraParameterFile': metadata_filepath,
		'VirtualCameraParameterFile': metadata_filepath,
		'InputCameraNames': input_cameras,
		'VirtualCameraNames': [virtual_camera],
		'ViewImageNames': view_image_names,
		'DepthMapNames': depth_map_names,
		'OutputFiles': output_files 
	}
	if input_overrides:
		configuration['InputOverrides'] = input_overrides

	# Write the configuration
	if not os.path.exists('config_files/{}'.format(sequence)):
		os.makedirs('config_files/{}' % sequence)

	filepath = 'config_files/{}/RVS-{}-{}.json'.format(sequence, test_class, virtual_camera)
	with open(filepath, 'w') as file:
		json.dump(configuration, file, indent=4)

for i, sequence in enumerate(sequences):
	for j, test_class in enumerate(test_classes[i]):
		# Source views
		for virtual_camera in source_views[i]:
			generate_configuration(sequence, test_class, anchor_coded_views[i][j], virtual_camera, {})

		# Intermediate views
		for virtual_camera in intermediate_views[i]:
			generate_configuration(sequence, test_class, anchor_coded_views[i][j], virtual_camera, {})

# Intermediate-view reference
for i, sequence in enumerate(sequences):
	for virtual_camera in source_views[i]:		
		generate_configuration(sequence, 'ref', source_views[i], virtual_camera, {'BitDepthDepth':16})
