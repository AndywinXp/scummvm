/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

int RenderDroid::LightModule::newEntry(RdLight *pLight) {
	pLight->type = 2;
	pLight->bActive = 1;
	pLight->direction[0] = 0.0f;
	pLight->direction[1] = 0.0f;
	pLight->direction[2] = 0.0f;
	pLight->intensity = 1.0f;
	pLight->color = 0xFFFFFF;
	pLight->angleX = 0.0f;
	pLight->cosAngleX = 0.0f;
	pLight->angleY = 0.0f;
	pLight->cosAngleY = 0.0f;

	return 1;
}

void RenderDroid::LightModule::setAngles(RdLight *pLight, float angleX, float angleY) {
	float unusedSin;

	pLight->angleX = angleX;
	pLight->angleY = angleY;
	_rd->_stdMath->sinCos(angleX, &unusedSin, &pLight->cosAngleX);
	_rd->_stdMath->sinCos(angleY, &unusedSin, &pLight->cosAngleY);

	double difference = (double)pLight->cosAngleX - pLight->cosAngleY;
	if (difference > 0.0)
		pLight->lux = (float)(1.0 / difference);
}

float RenderDroid::LightModule::calcVertexIntensities(RdLight **apLights, RdVector3 *aLightPos, RdVector3 *aLightDir, uint numLights, RdVector3 *aNormals, RdVector3 *aVertices, float *aStaticIntensities, float *aDynamicIntensities, uint numVertices, float attenuation) {
	float total = 0.0f;

	for (uint i = 0; i < numVertices; i++) {
		aDynamicIntensities[i] = aStaticIntensities[i];

		for (uint j = 0; j < numLights; j++) {
			RdLight *pLight = apLights[j];
			RdVector3 vec;
			double distanceSquared;

			if (pLight->type >= 2) {
				double dy = (double)aLightPos[j][1] - aVertices[i][1];
				double dx = (double)aLightPos[j][0] - aVertices[i][0];
				double dz = (double)aLightPos[j][2] - aVertices[i][2];

				vec[0] = (float)dx;
				vec[1] = (float)dy;
				vec[2] = (float)dz;
				distanceSquared = dx * vec[0] + dy * vec[1] + dz * vec[2];
			} else {
				distanceSquared = 0.0;
			}

			if (distanceSquared < (double)pLight->falloffMin * pLight->falloffMin) {
				double contribution = 0.0;
				bool lit = false;

				switch (pLight->type) {
				case 0:
				case 1: {
					double dot = (double)aNormals[i][0] * aLightDir[j][0] + (double)aNormals[i][1] * aLightDir[j][1] + (double)aNormals[i][2] * aLightDir[j][2];

					if (dot > 0.0) {
						contribution = pLight->intensity * dot;
						lit = true;
					}
					break;
				}
				case 2: {
					float distance = _rd->normalize3Acc(vec);
					double dot = (double)aNormals[i][0] * vec[0] + (double)aNormals[i][1] * vec[1] + (double)aNormals[i][2] * vec[2];

					if (dot > 0.0) {
						contribution = ((double)pLight->intensity - (double)distance * attenuation) * dot;
						lit = true;
					}
					break;
				}
				case 3: {
					float distance = _rd->normalize3Acc(vec);
					float cosine = (float)((double)aLightDir[j][1] * vec[1] + (double)aLightDir[j][0] * vec[0] + (double)aLightDir[j][2] * vec[2]);

					if (cosine > pLight->cosAngleY) {
						double intensity = pLight->intensity;

						if (cosine < pLight->cosAngleX)
							intensity = intensity * (1.0 - ((double)pLight->cosAngleX - cosine) * pLight->lux);

						contribution = ((double)aNormals[i][1] * aLightDir[j][1] + (double)aNormals[i][0] * aLightDir[j][0] + (double)aNormals[i][2] * aLightDir[j][2]) * (intensity - (double)distance * attenuation);
						lit = true;
					}
					break;
				}
				default:
					break;
				}

				if (lit)
					aDynamicIntensities[i] = (float)(contribution + aDynamicIntensities[i]);
			}

			if (aDynamicIntensities[i] > 1.0)
				aDynamicIntensities[i] = 1.0f;

			if (aDynamicIntensities[i] == 1.0f)
				break;
		}

		total = (float)((double)total + aDynamicIntensities[i]);
	}

	return (float)((double)total / (double)numVertices);
}

float RenderDroid::LightModule::calcFaceIntensity(RdLight **apLights, RdVector3 *aLightPos, RdVector3 *aLightDir, uint numLights, RdFace3 *pFace, RdVector3 normal, RdVector3 *aVertices, float attenuation) {
	float total = 0.0f;

	for (uint i = 0; i < numLights; i++) {
		RdLight *pLight = apLights[i];

		if (pLight->bActive) {
			RdVector3 vec;
			int vertex = pFace->aVertices[0];

			vec[0] = (float)((double)aLightPos[i][0] - aVertices[vertex][0]);
			vec[1] = (float)((double)aLightPos[i][1] - aVertices[vertex][1]);
			vec[2] = (float)((double)aLightPos[i][2] - aVertices[vertex][2]);

			if ((double)pLight->falloffMin * pLight->falloffMin > (double)vec[0] * vec[0] + (double)vec[1] * vec[1] + (double)vec[2] * vec[2]) {
				float distance = _rd->normalize3Acc(vec);
				double dot = (double)normal[2] * vec[2] + (double)normal[1] * vec[1] + (double)vec[0] * normal[0];

				if (dot > 0.0)
					total = (float)(((double)pLight->intensity - (double)distance * attenuation) * dot + total);
			}
		}

		if (total >= 1.0)
			break;
	}

	return total;
}

} // End of namespace Grim
