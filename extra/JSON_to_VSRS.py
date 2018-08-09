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
import sys

#### Input Parameters ##########################################################

in_filepath = 'cams_unicorn.json'
out_filepath = 'cams_unicorn_vsrs_2.txt'

scale = 1e-3    

################################################################################


def rotz(yaw):
    return np.array( [ [ np.cos(yaw), -np.sin(yaw), 0 ], [np.sin( yaw ), np.cos(yaw), 0], [0,0,1] ])
    
def roty(pitch):
    return np.array( [ [ np.cos(pitch), 0, np.sin(pitch) ], [0,1,0], [-np.sin(pitch), 0, np.cos(pitch)] ])
    
def rotx(roll):
    return np.array( [ [1,0,0], [ 0, np.cos(roll), -np.sin(roll) ], [0, np.sin(roll), np.cos(roll)] ])

def EulerAnglesToRotationMatrix( EulerAngles ):
    yaw, pitch, roll = EulerAngles
    R = rotz(yaw).dot( roty(pitch) ).dot ( rotx(roll) )
    return R


def main():    
    # Read JSON data    
    with open(in_filepath) as infile:
        data = json.load(infile)
    cameras  = data['cameras']

    # Convert to VSRS format
    sections = []
    permute = np.array([ [0, 0, 1], [-1, 0, 0], [0, -1, 0] ] )
    for cam in cameras :
        Name               = cam['Name']
        F                  = cam['Focal']
        C                  = cam['Principle_point']
        Intrinsics         = np.array([ [F[0], 0, C[0]], [0, F[1], C[1]], [0, 0, 1] ] )
        Position           = np.array( cam['Position'] )
        PostitionVSRS      = np.transpose(permute).dot( (Position / scale) )
        EulerAngles        = np.array( cam['Rotation'] )
        RotationMatrix     = EulerAnglesToRotationMatrix( np.deg2rad( EulerAngles ) )
        RotationMatrixVSRS = np.transpose(permute).dot( np.transpose(RotationMatrix) ).dot( permute )
        Extrinsics         = np.zeros((3,4))
        Extrinsics[:,0:3]  = RotationMatrixVSRS
        Extrinsics[:,3]    = PostitionVSRS
        section            = { 'Name'      : Name,
                               'Intrinsics': Intrinsics,
                               'Extrinsics': Extrinsics }
        sections.append(section)            
        
    # Write VSRS data
    fmt='%4.8f'    
    with open(out_filepath, 'w') as outfile:
        for section in sections:
            outfile.write(section['Name'] + '\n')
            np.savetxt( outfile, section['Intrinsics'], fmt)
            np.savetxt( outfile, np.array([[0,0]]), fmt)
            np.savetxt( outfile, section['Extrinsics'], fmt)
            outfile.write('\n')

if __name__ == "__main__":
   main()            

