# CORTAO Reference Implementation v1.0.1
#
# MIT License
#
# Copyright (c) 2026 Daniel Isheden
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


import math
import struct


def decodeSphereMap(x, y):
	ex = x * 2.0 - 1.0
	ey = y * 2.0 - 1.0

	z = max(1.0 - ex * ex - ey * ey, 0.0)
	mul = 2.0 * math.sqrt(z)

	return ex * mul, ey * mul, z * 2.0 - 1.0


def dot(a, b):
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]


def writeDDSHeader(file, width, height, depth, dxgiFormat):

	header = struct.pack(
		'<I I I I I I I 11I 32s I I I I I',
		124,
		0x1 | 0x2 | 0x4 | 0x100 | 0x800000, # flags: DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PIXELFORMAT, DDSD_DEPTH
		height,
		width,
		0,
		depth,
		1,
		*([0] * 11),
		struct.pack(
			'<I I 4s I I I I I',
			32, 0x4, b'DX10', 0, 0, 0, 0, 0
		),
		0x1000, # caps: DDSCAPS_TEXTURE
		0x200000, # caps: DDSCAPS2_VOLUME
		0, 
		0,
		0
	)

	dx10Header = struct.pack(
		'<I I I I I',
		dxgiFormat, 4, 0, 1, 0
	)

	file.write(b'DDS ')
	file.write(header)
	file.write(dx10Header);


def generateLookupTable(directionResolution, radiusResolution, format, dds, outputFolder):

	if directionResolution <= 0:
		print("ERROR: directionResolution must be more than 0")
		return

	if radiusResolution <= 0:
		print("ERROR: radiusResolution must be more than 0")
		return
	
	(rayCount, formatName, dxgiFormat) = format
	
	
	maxOccluderRadius = math.sqrt(2.0)

	print("\nGenerating a " + str(directionResolution) + "x" + str(directionResolution) + "x" + str(radiusResolution) + " with format " + formatName + " lookup table for " + str(rayCount) + " rays.")

	wordCount = (rayCount + 31) // 32

	if rayCount <= 8:
		format = "<B"
	elif rayCount <= 16:
		format = "<H"
	else:
		format = "<I"

	print("Precomputing ray directions...")
	goldenAngle = math.pi * (3.0 - math.sqrt(5.0))
	rayDirections = []
	for i in range(rayCount):
		angle = i * goldenAngle
		radius = math.sqrt((i + 0.5) / rayCount)

		x = math.cos(angle) * radius
		y = math.sin(angle) * radius
		z = math.sqrt(1.0 - x*x - y*y)

		rayDirections.append((x, y, z))

	print("Generating lookup table...")


	output = []

	for z in range(radiusResolution):
		occluderRadius = maxOccluderRadius * (z + 0.5) / radiusResolution

		occluderCosAngle = 1.0 - occluderRadius * occluderRadius * 0.5

		closestPixel = [(-1, -1, -1.0)] * rayCount
		offset = len(output)
		for y in range(directionResolution):
			for x in range(directionResolution):

				occluderDirection = decodeSphereMap((x + 0.5) / directionResolution, (y + 0.5) / directionResolution)

				rayBits = [0] * wordCount

				if occluderDirection[2] != -1.0:
					for i in range(rayCount):
						cosAngle = dot(occluderDirection, rayDirections[i])
						if cosAngle > closestPixel[i][2]:
							closestPixel[i] = (x, y, cosAngle)
						if cosAngle > occluderCosAngle:
							rayBits[i >> 5] |= 1 << (i&31)

				output.extend(rayBits)

		for i in range(rayCount):
			x, y, dist = closestPixel[i]
			index = y * directionResolution + x

			output[offset + index * wordCount + (i >> 5)] |= 1 << (i&31)
	if rayCount >= 32:
		numErrors = 0
		for bits in output:
			if (bits & 0x7F800000) == 0x7F800000:
				numErrors += 1

		if numErrors > 0:
			print("This lookup table cannot be sampled as a float texture, as it contains " + str(numErrors) + " entries with bit patterns interpreted as NaN. Some GPUs crush the mantissa bits to 0 for NaNs.")

	extension = ".dds" if dds else ".bin"

	fileName = str(directionResolution) + "x" + str(directionResolution) + "x" + str(radiusResolution) + "x" + str(rayCount) + "_" + formatName + extension

	filePath = outputFolder + fileName

	print("Writing file " + filePath)

	with open(filePath, "wb") as file:

		if dds:
			writeDDSHeader(file, directionResolution, directionResolution, radiusResolution, dxgiFormat)

		for bits in output:
			#print(bits)
			file.write(struct.pack(format, bits))

	print("Done!")


#Formats supported by this script
#(ray count, name, DXGI_FORMAT enum value)
formats = [
	#Ideal UINT formats
	(8, "R8_UINT", 62),
	(16, "R16_UINT", 57),
	(32, "R32_UINT", 42),
	
	# UINT formats with more than 32 rays. Not supported by the main pass shader at this moment, but theoretically doable.
	# (64, "R32G32_UINT", 17),
	# (128, "R32G32B32A32_UINT", 3),
	
	#UNORM workarounds for lack of Shader Model 6.7 support
	(8, "R8_UNORM", 61),
	(16, "R16_UNORM", 56)
]

#(disk resolution, rayResolution)
commonResolutions = [
	(24, 16), # For up to 32 samples
	(32, 24), # For up to 64 samples
	(64, 32)  # For up to 128 samples
]


#Simple example of generating all possible combinations of settings of the commonly used resolutions:

outputFolder = ""

for dds in [False, True]:
	for format in formats:
		for (directionResolution, radiusResolution) in commonResolutions:
			generateLookupTable(directionResolution, radiusResolution, format, dds, outputFolder)

