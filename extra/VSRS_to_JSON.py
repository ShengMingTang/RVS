#!/usr/bin/python

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
#   Sarah Fachada, Sarah.Fernandes.Pinto.Fachada@ulb.ac.be
#   Daniele Bonatto, Daniele.Bonatto@ulb.ac.be
#   Arnaud Schenkel, arnaud.schenkel@ulb.ac.be
# 
# Koninklijke Philips N.V., Eindhoven, The Netherlands:
#   Bart Kroon, bart.kroon@philips.com
#   Bart Sonneveldt, bart.sonneveldt@philips.com


import numpy as np
import json

#### Input Parameters ##########################################################

in_filepath = 'cams_unicorn_vsrs.txt'

out_filepath = 'cams_unicorn.json'

metadata = {
            'Version': '2.0', 
            'Content_name': 'ULB_Unicorn',
            'BoundingBox_center': [0, 0, 0],
            'Fps': 1,
            'Frames_number': 1,
            'Informative': {
                'Converted_by': 'VSRS_to_JSON.py',
                'Original_units': 'mm', 
                'New_units': 'm' },
            'cameras': {
                'Resolution': [1920, 1080],
                'Depth_range': [0.5, 2.0],       # meters
                'BitDepthColor': 8,
                'BitDepthDepth': 16 }
            }  

scale = 1e-3    


################################################################################





def AllmostZero(v):
    eps = 1e-7
    return abs(v) < eps

def RotationMatrixToEulerAngles(R):
    yaw   = 0.0
    pitch = 0.0
    roll  = 0.0
    
    if AllmostZero( R[0,0] ) and AllmostZero( R[1,0] ) :
        yaw = np.arctan2( R[1,2], R[0,2] )
        if R[2,0] < 0.0:
            pitch = np.pi/2
        else:
            pitch = -np.pi/2
        roll = 0.0
    else:
        yaw = np.arctan2( R[1,0], R[0,0] )
        if AllmostZero( R[0,0] ) :
            pitch = np.arctan2( -R[2,0], R[1,0] / np.sin(yaw) )
        else:
            pitch = np.arctan2( -R[2,0], R[0,0] / np.cos(yaw) )
        
        roll = np.arctan2( R[2,1], R[2,2] )
    
    euler = np.array( [yaw, pitch, roll] )
    
    return np.rad2deg(euler)
    


def main():
    sectionLength = 9
    permute = np.array([ [0, 0, 1], [-1, 0, 0], [0, -1, 0] ] )
    cameras = []            

    with open(in_filepath) as f:
        lines = f.readlines()
      
    l        = [l.strip() for l in lines] 
    sections = len(l) // sectionLength

    for section in range(sections):
        Name             = l[0]
        if not Name:
            break
        Intrinsics       = np.fromstring( l[1] + ' ' + l[2] + ' ' + l[3], dtype=float, sep=' ' )
        Intrinsics.shape = (3,3)
        Focal            = [Intrinsics[0,0], Intrinsics[1,1] ]
        Principle_point  = [Intrinsics[0,2], Intrinsics[1,2] ]
        
        Extrinsics       = np.fromstring( l[5] + ' ' + l[6] + ' ' + l[7], dtype=float, sep=' ' )
        Extrinsics.shape = (3,4)
        RotationMatrix   = np.transpose( permute.dot( Extrinsics[:,0:3] ).dot( np.transpose(permute) ) )
        Position         = scale * permute.dot(Extrinsics[:,3])
        EulerAngles      = RotationMatrixToEulerAngles(RotationMatrix);

        cam = {   'Name': Name,
                'Position': Position.tolist(), 
                'Rotation': EulerAngles.tolist(),
                'Depthmap': 1,
                'Background': 0,
                'Depth_range': metadata['cameras']['Depth_range'],
                'Resolution': metadata['cameras']['Resolution'],
                'Projection': 'Perspective',
                'Focal': Focal,
                'Principle_point': Principle_point,
                'BitDepthColor': metadata['cameras']['BitDepthColor'],
                'BitDepthDepth': metadata['cameras']['BitDepthDepth'],
                'ColorSpace': 'YUV420',
                'DepthColorSpace': 'YUV420' }

        cameras.append(cam)
        
        del l[0:sectionLength]
        print('%d %s' %(section, Name) )

       
    metadata['cameras'] = cameras      

    with open( out_filepath, 'w') as outfile:
        json.dump(metadata, outfile, indent=4)


if __name__ == "__main__":
   main()

