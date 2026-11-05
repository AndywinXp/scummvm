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

#ifndef GRIM_RENDERDROID_RD_H
#define GRIM_RENDERDROID_RD_H

#include "common/scummsys.h"

#include "engines/grim/std/std_conffile.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

typedef float RdVector3[3];
typedef float RdVector2[2];
typedef float RdMatrix34[4][3];

struct RdModel3;
struct RdModel3Mesh;
struct RdModel3HNode;
struct RdFace3;
struct RdMaterial;
struct RdTexture;
struct RdMatCel;
struct RdThing;
struct RdPuppet;
struct RdKeyframe;
struct RdCamera;
struct RdCanvas;
struct RdLight;
struct RdSprite;
struct RdPolyline;
struct RdParticleCloud;
struct RdProcFace;
struct RdActiveEdge;
struct RdActiveFace;
struct RdActiveSpan;

struct RdBound3 {
	RdVector3 min;
	RdVector3 max;
};

struct RdModel3Geo {
	uint numMeshes;
	RdModel3Mesh *aMeshes;
};

struct RdFace3 {
	uint num;
	uint type;
	int geometryMode;
	int lightingMode;
	int textureMode;
	uint numVertices;
	int *aVertices;
	int *aTexVertices;
	RdMaterial *pMaterial;
	int curCel;
	RdVector2 uvDelta;
	float extraLight;
	byte unk_34[12];
	RdVector3 normal;
};

struct RdModel3Mesh {
	char aName[32];
	uint num;
	int geometryMode;
	int lightingMode;
	int textureMode;
	RdVector3 *aVertices;
	RdVector2 *aTexVertices;
	float *aStaticVertexIntensities;
	float *aDynamicVertexIntensities;
	RdFace3 *aFaces;
	RdVector3 *aVertexNormals;
	uint numVertices;
	uint numTexVertices;
	uint numFaces;
	int castShadow;
	RdVector3 *aShadowVerts;
	float userLightValue;
	float radius;
	RdBound3 bound;
};

struct RdModel3HNode {
	char aName[64];
	uint flags;
	uint num;
	uint type;
	int meshNodeIndex;
	uint depth;
	RdModel3HNode *pParentNode;
	uint numChildren;
	RdModel3HNode *pFirstChildNode;
	RdModel3HNode *pSiblingNode;
	RdVector3 pivot;
	RdVector3 xyz;
	RdVector3 pyr;
	RdMatrix34 mat;
};

struct RdModel3 {
	char aName[32];
	int index;
	RdModel3Geo aGeo[4];
	uint numGeos;
	RdMaterial **apMaterials;
	uint numMaterials;
	uint curGeoSet;
	uint numHNodes;
	RdModel3HNode *aHNodes;
	float radius;
	RdBound3 bound;
	RdVector3 geomCenter;
	RdVector3 insertOffset;
};

struct RdMatCel {
	uint flags;
	int colorIndex;
	int unk_8;
	int unk_12;
	int unk_16;
	int unk_20;
	int unk_24;
	RdTexture *pTexture;
};

struct RdTexCacheEntry {
	void *pD3DTexture;
	void *pDDSurface;
	byte ddsd[108];
	uint hTexture;
	int bInCache;
	int bColorKey;
	uint width;
	uint height;
	uint size;
	uint lastFrame;
	RdTexCacheEntry *pPrev;
	RdTexCacheEntry *pNext;
};

struct RdTexture {
	uint flags;
	int unk_4;
	int colorKey;
	int widthLog2;
	int widthMask;
	int heightMask;
	uint numMips;
	StdVBuffer *apMips[4];
	RdTexCacheEntry aCache[2][4];
};

struct RdMaterial {
	uint flags;
	char aName[32];
	int unk_36;
	StdColorFormat format;
	uint numCels;
	int curCel;
	RdMatCel *apCels[16];
	uint numTextures;
	RdTexture *aTextures;
};

struct RdLight {
	int id;
	int type;
	int bActive;
	RdVector3 direction;
	float intensity;
	uint color;
	float angleX;
	float cosAngleX;
	float angleY;
	float cosAngleY;
	float lux;
	float falloffMin;
	float falloffMax;
};

struct RdVertexLists {
	uint numVertices;
	int *aVertIndices;
	int *aTexIndices;
	RdVector3 *aVertices;
	RdVector2 *aTexVertices;
	float *aIntensities;
	float *aAddedIntensities;
	byte unk_1C[12];
	RdVector3 *aScreen;
};

struct RdClipPoly {
	RdVector3 *aVerts;
	RdVector2 *aTexVerts;
	float *aIntensities;
};

struct RdKeyframeEntry {
	float frame;
	uint flags;
	RdVector3 pos;
	RdVector3 pyr;
	RdVector3 dpos;
	RdVector3 dpyr;
};

struct RdKeyframeNode {
	char aName[32];
	uint nodeNum;
	uint numEntries;
	RdKeyframeEntry *paEntries;
};

struct RdKeyframe {
	char aName[32];
	int unk_32;
	uint flags;
	uint numJoints;
	uint type;
	float fps;
	uint numFrames;
	uint numNodes;
	RdKeyframeNode *paNodes;
	uint numMarkers;
	float aMarkerFrame[8];
	int aMarkerType[8];
};

typedef void (*RdPuppetTrackCallback)(int userArg, int trackNum, int markerType);

struct RdPuppetTrack {
	uint status;
	int unk_4;
	int priority;
	int priorityMasked;
	float fps;
	float unk_20;
	float fade;
	float fadeSpeed;
	int aEntryIdx[64];
	float curFrame;
	float prevFrame;
	RdKeyframe *pKeyframe;
	RdPuppetTrackCallback pfCallback;
	int unk_304;
};

struct RdPuppet {
	int bPaused;
	RdThing *pThing;
	RdPuppetTrack aTracks[6];
};

struct RdCanvas {
	uint flags;
	StdVBuffer *pVBuffer;
	float xCenter;
	float yCenter;
	StdVBuffer *pZBuffer;
	int zScaleShift;
	int xStart;
	int yStart;
	int xEnd;
	int yEnd;
};

struct RdClipFrustum {
	int bClipFar;
	float zNear;
	float zFar;
	float orthoLeft;
	float orthoTop;
	float orthoRight;
	float orthoBottom;
	float top;
	float bottom;
	float left;
	float right;
	float guardTop;
	float guardLeft;
};

struct RdSprite {
	char aName[32];
	int type;
	float radius;
	byte unk_40[36];
	float width;
	float height;
	float halfWidth;
	float halfHeight;
	int unk_92;
	int bFlag;
	int geometryMode;
	int lightingMode;
	int textureMode;
	uint numVerts;
	int *aVertexIdx;
	int *aTexVertexIdx;
	RdMaterial *pMaterial;
	int unk_128;
	RdVector2 uvDelta;
	float extraLight;
	byte unk_144[24];
	float *aTexVerts;
	RdVector3 offset;
};

struct RdPolyline {
	char aName[32];
	float length;
	float startWidth;
	float endWidth;
	byte unk_44[12];
	RdFace3 faceBody;
	RdFace3 faceEnd;
	RdVector2 *aTexVertsBody;
	RdVector2 *aTexVertsEnd;
};

struct RdParticleCloud {
	char aName[32];
	int lightingMode;
	uint numParticles;
	RdVector3 *aPositions;
	int *aCels;
	int unk_48;
	float size;
	RdMaterial *pMaterial;
	float radius;
};

struct RdThing {
	int type;
	union {
		RdModel3 *pModel3;
		RdCamera *pCamera;
		RdLight *pLight;
		RdSprite *pSprite;
		RdPolyline *pPolyline;
		RdParticleCloud *pParticle;
		void *pData;
	} data;
	int geometryMode;
	int lightingMode;
	int textureMode;
	RdPuppet *pPuppet;
	int bJointsLocked;
	int jointsFrame;
	RdMatrix34 *aJointMatrices;
	RdVector3 *aJointPyr;
	int *abJointAmputate;
	int *abMeshVisible;
	int curCel;
	int curGeoSet;
	int curGeometryMode;
	int curLightingMode;
	int curTextureMode;
	int frustrumCull;
	int userArg;
};

struct RdColormap {
	char aName[32];
	uint flags;
	RdVector3 tint;
	byte aColors[768];
	byte *pLightLevel;
	byte *pLightLevelAlloc;
	byte *pTransparency;
	byte *pTransparencyAlloc;
	int unk_832;
	int unk_836;
	byte *pRGB16;
	byte *pRGB16Alpha;
};

typedef void (*RdProcFaceDrawFunc)(RdProcFace *pFace, RdMatCel *pCel, float light);
typedef void (*RdProcFaceSetupFunc)(RdActiveFace *pFace, RdMatCel *pCel, float light);

struct RdProcFace {
	uint flags;
	int type;
	int geometryMode;
	int lightingMode;
	int textureMode;
	RdProcFaceDrawFunc pfUserDraw;
	RdProcFaceSetupFunc pfUserSetup;
	uint numVertices;
	RdVector3 *aVertices;
	RdVector2 *aTexVertices;
	float *aIntensities;
	float *aRedIntensities;
	float *aGreenIntensities;
	float *aBlueIntensities;
	RdMaterial *pMaterial;
	int curCel;
	float ambientLight;
	float diffuseLight;
	float extraLight;
	RdColormap *pColormap;
	int bBackface;
	int xMin;
	int xMax;
	int yMin;
	int yMax;
	float zMin;
	float zMax;
	int yMinIdx;
	int yMaxIdx;
	int userData;
};

struct RdColorEffects {
	int tint[3];
	int filter[3];
	int add[3];
	float fade;
};

typedef RdMaterial *(*RdMaterialLoaderFunc)(char *path, int a2, int a3);
typedef int (*RdMaterialUnloaderFunc)(RdMaterial *pMaterial);

struct RdNRasterEdge {
	int x;
	int dx;
	int numLines;
	uint vertIdx;
	float oneOverZ;
	float dOneOverZ;
	float uOverZ;
	float dUOverZ;
	float vOverZ;
	float dVOverZ;
	int u;
	int du;
	int v;
	int dv;
	int intensity;
	int dIntensity;
	int y;
};

typedef RdNRasterEdge RdZRasterEdge;

class RenderDroid {
public:
	RenderDroid(StdMath *stdMath, StdDisplay *stdDisplay, StdConffile *stdConffile, StdFile *stdFile);
	~RenderDroid();

	int startup(SysStruct *pHS);
	void shutdown();
	int open(int acceleration);
	int close();
	int setRenderOptions(int options);
	int setGeometryMode(int mode);
	int setLightingMode(int mode);
	int setTextureMode(int mode);
	int setSortingMethod(int method);
	int setOcclusionMethod(int method);
	int setZBufferMethod(int method);
	int advanceFrame();
	void finishFrame();
	void clearPreStatistics();
	int beginHardwareScene();
	void setColorInfo(const StdColorFormat &colorInfo);

	void cross3(RdVector3 out, RdVector3 a, RdVector3 b);
	float dot2(RdVector2 a, RdVector2 b);
	float dot3(RdVector3 a, RdVector3 b);
	void extractAngles(RdVector3 vec, RdVector3 pyr);
	float normalize3(RdVector3 out, RdVector3 in);
	float normalize3Acc(RdVector3 vec);
	float normalize3QuickAcc(RdVector3 vec);
	void rotate3Acc(RdVector3 vec, RdVector3 pyr);

	void calcSurfaceNormal(RdVector3 out, RdVector3 p0, RdVector3 p1, RdVector3 p2);
	void clampVector(RdVector3 vec, float eps);
	int pointsCollinear(RdVector3 a, RdVector3 b, RdVector3 c);

	void build34(RdMatrix34 out, RdVector3 pyr, RdVector3 pos);
	void buildFromVectorAngle34(RdMatrix34 out, RdVector3 axis, float angle);
	void buildRotate34(RdMatrix34 out, RdVector3 pyr);
	void buildScale34(RdMatrix34 out, RdVector3 scale);
	void buildTranslate34(RdMatrix34 out, RdVector3 pos);
	void extractAngles34(RdMatrix34 mat, RdVector3 pyr);
	void identity34(RdMatrix34 out);
	void invertOrtho34(RdMatrix34 out, RdMatrix34 in);
	void lookAt(RdMatrix34 out, RdVector3 pos, RdVector3 target, float roll);
	void multiply34(RdMatrix34 out, RdMatrix34 a, RdMatrix34 b);
	void postMultiply34(RdMatrix34 dst, RdMatrix34 src);
	void postTranslate34(RdMatrix34 mat, RdVector3 vec);
	void preMultiply34(RdMatrix34 dst, RdMatrix34 src);
	void preRotate34(RdMatrix34 mat, RdVector3 pyr);
	void preScale34(RdMatrix34 mat, RdVector3 scale);
	void transformPoint34(RdVector3 out, RdVector3 pt, RdMatrix34 mat);
	void transformPoint34Acc(RdVector3 pt, RdMatrix34 mat);
	void transformPointLst34(RdMatrix34 mat, RdVector3 *aIn, RdVector3 *aOut, uint num);
	void transformVector34(RdVector3 out, RdVector3 vec, RdMatrix34 mat);
	void transformVector34Acc(RdVector3 vec, RdMatrix34 mat);
	void transformVectorOrtho34(RdVector3 out, RdVector3 vec, RdMatrix34 mat);

	class Module {
	protected:
		explicit Module(RenderDroid *rd) : _rd(rd) {}
		RenderDroid *_rd;
	};

	class CameraModule : public Module {
	public:
		explicit CameraModule(RenderDroid *rd) : Module(rd), _pCurCamera(nullptr) {}
		RdCamera *getCurrent() const { return _pCurCamera; }
		void getPosition(RdVector3 out) const { out[0] = _camMatrix[3][0]; out[1] = _camMatrix[3][1]; out[2] = _camMatrix[3][2]; }
		RdCamera *create(float fov, int bClipFar, float zNear, float zFar, float aspectRatio);
		int newEntry(RdCamera *pCamera, float fov, int bClipFar, float zNear, float zFar, float aspectRatio);
		void destroy(RdCamera *&pCamera);
		void freeEntry(RdCamera *pCamera);
		int setCanvas(RdCamera *pCamera, RdCanvas *pCanvas);
		int setCurrent(RdCamera *pCamera);
		int setFOV(RdCamera *pCamera, float fov);
		int setProjectType(RdCamera *pCamera, int projectType);
		int setAspectRatio(RdCamera *pCamera, float aspectRatio);
		int buildFOV(RdCamera *pCamera);
		void update(RdMatrix34 mat);
		void orthoProject(RdVector3 pOut, RdVector3 pIn);
		void orthoProjectLst(RdVector3 *aOut, RdVector3 *aIn, int num);
		void orthoProjectSquare(RdVector3 pOut, RdVector3 pIn);
		void orthoProjectSquareLst(RdVector3 *aOut, RdVector3 *aIn, int num);
		void perspProject(RdVector3 pOut, RdVector3 pIn);
		void perspProjectLst(RdVector3 *aOut, RdVector3 *aIn, int num);
		void perspProjectSquare(RdVector3 pOut, RdVector3 pIn);
		void perspProjectSquareLst(RdVector3 *aOut, RdVector3 *aIn, int num);
		void setAmbientLight(RdCamera *pCamera, float ambient);
		int addLight(RdCamera *pCamera, RdLight *pLight, RdVector3 *pPos);
		void advanceFrame();

	private:
		friend class RenderDroid;
		RdCamera *_pCurCamera;
		RdMatrix34 _camMatrix;
		RdVector3 _camPYR;
	};

	class CanvasModule : public Module {
	public:
		explicit CanvasModule(RenderDroid *rd) : Module(rd) {}
		RdCanvas *create(uint flags, StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int a8);
		int newEntry(RdCanvas *pCanvas, uint flags, StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int a9);
		void destroy(RdCanvas *&pCanvas);
	};

	class ClipModule : public Module {
	public:
		explicit ClipModule(RenderDroid *rd) : Module(rd), _faceStatus(0) {}
		int line2(RdCanvas *pCanvas, int *x0, int *y0, int *x1, int *y1);
		int calcOutcode2(RdCanvas *pCanvas, int x, int y);
		int point3(RdClipFrustum *pClip, RdVector3 pt);
		int line3(RdClipFrustum *pClip, RdVector3 v0, RdVector3 v1, int *pClipped0, int *pClipped1);
		int face3W(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts);
		int face3WOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts);
		int face3S(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts);
		int face3SOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, int numVerts);
		int face3GS(RdClipFrustum *pClip, RdVector3 *aVerts, float *aIntensities, int numVerts);
		int face3GSOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, float *aIntensities, int numVerts);
		int face3T(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, int numVerts);
		int face3TOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, int numVerts);
		int face3GT(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts);
		int face3GTOrtho(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts);
		int sphereInFrustrum(RdClipFrustum *pClip, RdVector3 center, float radius);
		int faceToPlane(RdVector3 norm, RdVector3 pt, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts);

	private:
		friend class RenderDroid;
		int clipFace(RdClipFrustum *pClip, RdVector3 *aVerts, RdVector2 *aTexVerts, float *aIntensities, int numVerts, bool bGuard, bool bOrtho);
		int clipPerspectiveX(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float slope, bool keepGreater, uint status);
		int clipPerspectiveZ(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float slope, bool keepGreater, uint status);
		int clipOrtho(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, int axis, float bound, bool keepGreater, uint status);
		int clipDepth(const RdClipPoly *pSrc, int count, RdClipPoly *pDst, float plane, bool keepGreater, uint status);
		void copyVertex(const RdClipPoly *pSrc, int srcIdx, RdClipPoly *pDst, int dstIdx);
		void interpolateAttributes(const RdClipPoly *pSrc, int prevIdx, int curIdx, RdClipPoly *pDst, int dstIdx, double t);
		bool canAddVertex(int count);

		RdVector3 _aVertBufferA[64];
		RdVector2 _workTVerts[64];
		float _workIVerts[64];
		uint _faceStatus;
	};

	class ColormapModule : public Module {
	public:
		explicit ColormapModule(RenderDroid *rd) : Module(rd), _pCurMap(nullptr), _pIdentityMap(nullptr) {}
		RdColormap *getCurrent() const { return _pCurMap; }
		RdColormap *getIdentity() const { return _pIdentityMap; }
		RdColormap *load(char *path);
		int loadEntry(char *path, RdColormap *pColormap);
		void destroy(RdColormap *&pColormap);
		void freeEntry(RdColormap *pColormap);
		int setCurrent(RdColormap *pColormap);
		int setIdentity(RdColormap *pColormap);
		int buildRGB16(byte *pDst, byte *pPalette, byte r, byte g, byte b, StdColorFormat *pColorFormat);
		int buildGrayRamp(RdColormap *pColormap);
		int size(RdColormap *pColormap);

	private:
		friend class RenderDroid;
		RdColormap *_pCurMap;
		RdColormap *_pIdentityMap;
	};

	class ColorEffectsModule : public Module {
	public:
		explicit ColorEffectsModule(RenderDroid *rd) : Module(rd) {}
		void clear(RdColorEffects *pEffects);
	};

	class FaceModule : public Module {
	public:
		explicit FaceModule(RenderDroid *rd) : Module(rd) {}
		int newEntry(RdFace3 *pFace);
		void freeEntry(RdFace3 *pFace);
	};

	class KeyframeModule : public Module {
	public:
		explicit KeyframeModule(RenderDroid *rd) : Module(rd) {}
		RdKeyframe *load(char *path);
		int loadEntry(char *path, RdKeyframe *pKeyframe);
		int loadHeader(Common::SeekableReadStream *pFile, RdKeyframe *pKeyframe);
		void newEntry(RdKeyframe *pKeyframe);
		void destroy(RdKeyframe *&pKeyframe);
		void freeEntry(RdKeyframe *pKeyframe);
	};

	class LightModule : public Module {
	public:
		explicit LightModule(RenderDroid *rd) : Module(rd) {}
		int newEntry(RdLight *pLight);
		void setAngles(RdLight *pLight, float angleX, float angleY);
		float calcVertexIntensities(RdLight **apLights, RdVector3 *aLightPos, RdVector3 *aLightDir, uint numLights, RdVector3 *aNormals, RdVector3 *aVertices, float *aStaticIntensities, float *aDynamicIntensities, uint numVertices, float attenuation);
		float calcFaceIntensity(RdLight **apLights, RdVector3 *aLightPos, RdVector3 *aLightDir, uint numLights, RdFace3 *pFace, RdVector3 normal, RdVector3 *aVertices, float attenuation);
	};

	class MaterialModule : public Module {
	public:
		explicit MaterialModule(RenderDroid *rd) : Module(rd), _pfLoader(nullptr), _pfUnloader(nullptr) {}
		RdMaterialLoaderFunc registerLoader(RdMaterialLoaderFunc pfLoader);
		RdMaterialUnloaderFunc registerUnloader(RdMaterialUnloaderFunc pfUnloader);
		RdMaterial *load(char *path, int bVideoMemory, int bVideoMemoryOnly);
		int loadEntry(char *path, RdMaterial *pMaterial, int bVideoMemory, int bVideoMemoryOnly);
		int destroy(RdMaterial *&pMaterial);
		void freeEntry(RdMaterial *pMaterial);

	private:
		RdMaterialLoaderFunc _pfLoader;
		RdMaterialUnloaderFunc _pfUnloader;
	};

	class Model3Module : public Module {
	public:
		explicit Model3Module(RenderDroid *rd) : Module(rd), _pCurThing(nullptr), _pCurModel3(nullptr), _pCurGeo(nullptr), _pCurMesh(nullptr), _thingFrustrumCull(0), _meshFrustrumCull(0),
				_curGeometryMode(0), _curLightingMode(0), _curTextureMode(0), _geometryMode(0), _lightingMode(0), _textureMode(0),
				_numGeoLights(0), _numMeshLights(0), _numDrawnModels(0), _clipPlaneActive(0), _maxGeoRadius(0.0f) {
			memset(_apGeoLights, 0, sizeof(_apGeoLights));
			memset(_apMeshLights, 0, sizeof(_apMeshLights));
			memset(&_vertexSrc, 0, sizeof(_vertexSrc));
			memset(&_vertexDst, 0, sizeof(_vertexDst));
			memset(_clipNorm, 0, sizeof(_clipNorm));
			memset(_clipPt, 0, sizeof(_clipPt));
			memset(_localCamera, 0, sizeof(_localCamera));
			_clipNorm[2] = 1.0f;
		}
		void clearFrameCounters();
		void activateClipPlane(int onOff);
		void setClipPlane(RdVector3 norm, RdVector3 pt);
		int newEntry(RdModel3 *pModel3);
		RdModel3 *load(char *pFilename);
		int loadEntry(char *pFilename, RdModel3 *pModel3);
		int loadEntryText(char *pFilename, RdModel3 *pModel3);
		int loadEntryBinary(Common::SeekableReadStream *pFile, RdModel3 *pModel3);
		void loadPostProcess(RdModel3 *pModel3);
		void destroy(RdModel3 *&pModel3);
		void freeEntry(RdModel3 *pModel3);
		void buildShadowModel(RdModel3 *pModel3);
		void calcBoundingBoxes(RdModel3 *pModel3);
		void calcModelBoundingBox(RdModel3 *pModel3, RdModel3HNode *pHNode, RdMatrix34 parentMatrix);
		void calcRadii(RdModel3 *pModel3);
		void buildExpandedRadius(RdModel3 *pModel3, RdModel3HNode *pHNode, RdMatrix34 parentMatrix);
		void calcNodeDepths(RdModel3 *pModel3);
		void calcFaceNormals(RdModel3 *pModel3);
		void calcVertexNormals(RdModel3 *pModel3);
		int getMeshMatrix(RdThing *pThing, RdMatrix34 orient, uint nodeNum, RdMatrix34 meshOrient);
		int draw(RdThing *pThing, RdMatrix34 mat);
		void drawHNode(RdModel3HNode *pHNode);
		void drawMesh(RdModel3Mesh *pMesh, RdMatrix34 mat);
		int drawFace(RdFace3 *pFace, int bBackface);
		int drawPolyline(RdThing *pThing, RdMatrix34 mat);
		int drawParticle(RdThing *pThing, RdMatrix34 mat);

	private:
		friend class RenderDroid;
		int hasBytes(Common::SeekableReadStream *pFile, uint count, uint elementSize);

		RdThing *_pCurThing;
		RdModel3 *_pCurModel3;
		RdModel3Geo *_pCurGeo;
		RdModel3Mesh *_pCurMesh;
		int _thingFrustrumCull;
		int _meshFrustrumCull;
		int _curGeometryMode;
		int _curLightingMode;
		int _curTextureMode;
		int _geometryMode;
		int _lightingMode;
		int _textureMode;
		uint _numGeoLights;
		uint _numMeshLights;
		RdLight *_apGeoLights[64];
		RdLight *_apMeshLights[64];
		RdVector3 _aLocalLightPos[64];
		RdVector3 _aLocalLightDir[64];
		RdVector3 _aView[512];
		RdVector3 _aFaceVerts[32];
		RdVector3 _localCamera;
		RdVertexLists _vertexSrc;
		RdVertexLists _vertexDst;
		uint _numDrawnModels;
		int _clipPlaneActive;
		RdVector3 _clipNorm;
		RdVector3 _clipPt;
		float _maxGeoRadius;
		RdVector3 _particleTransformed[256];
		float _particleVerts[12];
		float _polylineVerts[12];
	};

	class Primit2Module : public Module {
	public:
		explicit Primit2Module(RenderDroid *rd) : Module(rd) {}
		void drawLine(RdCanvas *pCanvas, int x0, int y0, int x1, int y1, uint16 color, uint pattern);
		int drawClippedLine(RdCanvas *pCanvas, int x0, int y0, int x1, int y1, uint16 color, uint pattern);
		void drawCircle(RdCanvas *pCanvas, int cx, int cy, float radius, float angleStep, uint16 color, uint pattern);
		void drawRectangle(RdCanvas *pCanvas, int x0, int y0, int x1, int y1, uint16 color, uint pattern);

	private:
		void plot(RdCanvas *pCanvas, int x, int y, uint16 color);
		void drawBresenham(RdCanvas *pCanvas, int x0, int y0, int x1, int y1, uint16 color, uint pattern);
	};

	class Primit3Module : public Module {
	public:
		explicit Primit3Module(RenderDroid *rd) : Module(rd) {}
		RdProcFace *drawPolygon(RdVector3 *aVerts, int numVerts, RdMaterial *pMaterial);
		int getScreenCoord(RdVector3 pt, int *pScreen);
		int drawLine(RdVector3 p0, RdVector3 p1, int *pScreen0, int *pScreen1, int *pClipped0, int *pClipped1, uint16 color, uint pattern);
		void drawPoint(RdVector3 pt, float radius, float angleStep, uint16 color, uint pattern);
		int clipFace(RdClipFrustum *pClip, int geometryMode, int lightingMode, int textureMode, RdVertexLists *pSrc, RdVertexLists *pDst, RdVector2 uvDelta);
		int noClipFace(int geometryMode, int lightingMode, int textureMode, RdVertexLists *pSrc, RdVertexLists *pDst, RdVector2 uvDelta);
		void clearFrameCounters();
		int drawPolylineFace(RdThing *pThing, RdFace3 *pFace, RdVector3 *aVerts, RdVertexLists *pSrc);

	private:
		friend class RenderDroid;
		RdVector3 _work[32];
	};

	class PuppetModule : public Module {
	public:
		explicit PuppetModule(RenderDroid *rd) : Module(rd) {}
		RdPuppet *create(RdThing *pThing);
		int newEntry(RdPuppet *pPuppet, RdThing *pThing);
		int addTrack(RdPuppet *pPuppet, RdKeyframe *pKeyframe, int priority, int priorityMasked);
		int removeTrack(RdPuppet *pPuppet, int trackNum);
		int setStatus(RdPuppet *pPuppet, int trackNum, uint status);
		void setCallback(RdPuppet *pPuppet, int trackNum, RdPuppetTrackCallback pfCallback);
		int playTrack(RdPuppet *pPuppet, int trackNum);
		int fadeInTrack(RdPuppet *pPuppet, int trackNum, float time);
		int fadeOutTrack(RdPuppet *pPuppet, int trackNum, float time);
		void advanceTrack(RdPuppet *pPuppet, int trackNum, float delta);
		int updateTracks(RdPuppet *pPuppet, float delta);
		int resetTrack(RdPuppet *pPuppet, int trackNum);
		void buildJointMatrices(RdThing *pThing, RdMatrix34 pBase);
	};

	class SpriteModule : public Module {
	public:
		explicit SpriteModule(RenderDroid *rd) : Module(rd) {}
		RdSprite *create(int type, char *name, char *matFilename, float width, float height, int geometryMode, int lightingMode, int textureMode, float extraLight, RdVector3 *pOffset);
		int newEntry(RdSprite *pSprite, char *name, int type, char *matFilename, float width, float height, int geometryMode, int lightingMode, int textureMode, float extraLight, RdVector3 *pOffset);
		void destroy(RdSprite *&pSprite);
		void freeEntry(RdSprite *pSprite);
		int draw(RdThing *pThing, RdMatrix34 mat);

	private:
		friend class RenderDroid;
		float _spriteVerts[12];
		RdVector3 _spriteWork[32];
	};

	class ThingModule : public Module {
	public:
		explicit ThingModule(RenderDroid *rd) : Module(rd) {}
		RdThing *create(int userArg);
		int newEntry(RdThing *pThing, int userArg);
		void destroy(RdThing *&pThing);
		void freeEntry(RdThing *pThing);
		int setModel3(RdThing *pThing, RdModel3 *pModel3);
		int setCamera(RdThing *pThing, RdCamera *pCamera);
		int setSprite3(RdThing *pThing, RdSprite *pSprite);
		int draw(RdThing *pThing, RdMatrix34 mat);
		void accumulateMatrices(RdThing *pThing, RdModel3HNode *pHNode, RdMatrix34 pParent);
	};

	class ActiveModule : public Module {
	public:
		explicit ActiveModule(RenderDroid *rd);
		~ActiveModule();
		int startup();
		int advanceFrame();
		void clearFrameCounters();
		uint drawScene();
		int buildSpans();
		uint flushSpans();
		uint buildEdges();
		int addActiveFace(RdProcFace *pProcFace);

	private:
		friend class RenderDroid;
		void addSpan(RdActiveFace *pFace, uint alignedEndX);
		RdActiveEdge *_aActiveEdges;
		RdActiveFace *_aActiveFaces;
		RdActiveSpan *_aActiveSpans;
		RdActiveEdge *_pActiveEdgeHead;
		RdActiveEdge *_pActiveEdgeTail;
		RdActiveEdge *_apNewActiveEdges[1024];
		RdActiveEdge *_apRemoveActiveEdges[1024];
		uint _drawnFaces;
		uint _numActiveEdges;
		uint _numActiveFaces;
		uint _numActiveSpans;
		uint _yCurScanLine;
		uint _yMaxEdge;
		uint _yMinEdge;
	};

	class CacheModule : public Module {
	public:
		explicit CacheModule(RenderDroid *rd) : Module(rd), _numProcFaces(0), _numUsedVertices(0), _numUsedTexVertices(0), _numUsedIntensities(0), _drawnFaces(0) {
			memset(_viewportRect, 0, sizeof(_viewportRect));
		}
		int *getViewportRect() { return _viewportRect; }
		RdProcFace *getProcFaces() { return _aProcFaces; }
		size_t getNumProcFaces() const { return _numProcFaces; }
		RdMatCel *selectCel(const RdProcFace *pProcFace);
		float calcFaceLight(const RdProcFace *pProcFace, int lightingMode, float ambientLight);
		void calcVertexLights(RdProcFace *pProcFace, float ambientLight);
		int startup();
		int finishFrame();
		int reset();
		void clearFrameCounters();
		RdProcFace *getProcEntry();
		int flush();
		int addProcFace(uint flags, uint numVertices, char bufferFlags);
		int drawFaceN(RdProcFace *pProcFace);
		int drawFaceZ(RdProcFace *pProcFace);
		int drawFaceUser(RdProcFace *pProcFace);
		int procFaceCompare(const RdProcFace *pA, const RdProcFace *pB);

	private:
		friend class RenderDroid;
		float _aVertices[98304];
		float _aTexVertices[65536];
		float _aIntensities[32768];
		float _aGreenIntensities[32768];
		float _aBlueIntensities[32768];
		RdProcFace _aProcFaces[1024];
		size_t _numProcFaces;
		uint _numUsedVertices;
		uint _numUsedTexVertices;
		uint _numUsedIntensities;
		uint _drawnFaces;
		int _viewportRect[4];
	};

	class RasterModule : public Module {
	public:
		explicit RasterModule(RenderDroid *rd) : Module(rd), _fixedScale(65536.0f) {}
		void startup();
		float getOneOverNFlex(int index) const { return _aOneOverNFlex[index]; }
		int getOneOverNFixed(int index) const { return _aOneOverNFixed[index]; }

	private:
		friend class RenderDroid;
		int _aOneOverNFixed[2048];
		float _aOneOverNFixedF[16];
		float _aOneOverNFlex[2048];
		float _fixedScale;
	};

	class AFRasterModule : public Module {
	public:
		typedef void (AFRasterModule::*DrawSpanFunc)(RdActiveSpan *pSpan);
		typedef void (AFRasterModule::*DrawFaceFunc)(RdActiveFace *pFace);
		typedef void (AFRasterModule::*StepEdgeFunc)(RdActiveEdge *pEdge);
		typedef int (AFRasterModule::*SetupEdgeFunc)(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);

		explicit AFRasterModule(RenderDroid *rd) : Module(rd), _mipMapNum(0), _texRowShift(0), _texPitch(0), _uWrap(0), _vWrap(0), _dUdXModifier(0), _dVdXModifier(0), _dOneOverZdXFixed(0), _dOneOverZdX(0.0f), _dUOverZdX(0.0f), _dVOverZdX(0.0f), _oneOverZStep(0.0f), _uOverZStep(0.0f), _vOverZStep(0.0f), _pPixels(nullptr), _aMonoMap(nullptr), _aRGBMap(nullptr), _color(0), _RGBcolor(0), _pfDrawSpan(nullptr) {}

		void calcAffineGradients(RdActiveFace *pFace, int refVert);
		void calcPerspGradients(RdActiveFace *pFace, int refVert);

		void setupNGonLW(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonLS(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonFS(RdActiveFace *pFace, RdMatCel *pCel, float light);
		void setupNGonGS(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonLAT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonMLAT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonFAT(RdActiveFace *pFace, RdMatCel *pCel, float light);
		void setupNGonMFAT(RdActiveFace *pFace, RdMatCel *pCel, float light);
		void setupNGonGAT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonMGAT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonLIT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonMLIT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonFIT(RdActiveFace *pFace, RdMatCel *pCel, float light);
		void setupNGonMFIT(RdActiveFace *pFace, RdMatCel *pCel, float light);
		void setupNGonGIT(RdActiveFace *pFace, RdMatCel *pCel);
		void setupNGonMGIT(RdActiveFace *pFace, RdMatCel *pCel);

		void drawNGonLW(RdActiveFace *pFace);
		void drawNGonLS(RdActiveFace *pFace);
		void drawNGonFS(RdActiveFace *pFace);
		void drawNGonGS(RdActiveFace *pFace);
		void drawNGonLAT(RdActiveFace *pFace);
		void drawNGonMLAT(RdActiveFace *pFace);
		void drawNGonFAT(RdActiveFace *pFace);
		void drawNGonMFAT(RdActiveFace *pFace);
		void drawNGonGAT(RdActiveFace *pFace);
		void drawNGonMGAT(RdActiveFace *pFace);
		void drawNGonLIT(RdActiveFace *pFace);
		void drawNGonMLIT(RdActiveFace *pFace);
		void drawNGonFIT(RdActiveFace *pFace);
		void drawNGonMFIT(RdActiveFace *pFace);
		void drawNGonGIT(RdActiveFace *pFace);
		void drawNGonMGIT(RdActiveFace *pFace);

		int setupEdgeNGonLW(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonLS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonFS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonGS(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonLAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonFAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonGAT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonLIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonFIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
		int setupEdgeNGonGIT(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);

		void advanceEdgeNGonLS(RdActiveEdge *pEdge);
		void advanceEdgeNGonGS(RdActiveEdge *pEdge);
		void advanceLeftEdgeNGonFAT(RdActiveEdge *pEdge);
		void advanceLeftEdgeNGonGAT(RdActiveEdge *pEdge);
		void advanceLeftEdgeNGonFIT(RdActiveEdge *pEdge);
		void advanceRightEdgeNGonLIT(RdActiveEdge *pEdge);
		void advanceEdgeNGonGIT(RdActiveEdge *pEdge);
		void advanceLeftEdgeNGonGIT(RdActiveEdge *pEdge);

		void drawSpanNGonLW_8(RdActiveSpan *pSpan);
		void drawSpanNGonLW_16(RdActiveSpan *pSpan);
		void drawSpanNGonMLAT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMLAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonMFAT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMFAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonMGAT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMGAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonMLIT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMLIT_16(RdActiveSpan *pSpan);
		void drawSpanNGonMFIT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMFIT_16(RdActiveSpan *pSpan);
		void drawSpanNGonMGIT_8(RdActiveSpan *pSpan);
		void drawSpanNGonMGIT_16(RdActiveSpan *pSpan);

		void drawSpanNGonLAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonLAT_16_W256(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonFAT_16_W256(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonGAT_16_W256(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonLIT_16_W256(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonFIT_16_W256(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W8(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W16(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W32(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W64(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W128(RdActiveSpan *pSpan);
		void drawSpanNGonGIT_16_W256(RdActiveSpan *pSpan);

		void drawZSpan(RdActiveSpan *pSpan);
		void drawZSpanShifted(RdActiveSpan *pSpan);

	private:
		friend class RenderDroid;

		struct SpanRun {
			byte *pDst;
			uint16 *pZ;
			int z;
			int dz;
			int zShift;
			int u;
			int v;
			int du;
			int dv;
			int intensity;
			int dIntensity;
			int uMask;
			int vShift;
		};

		int wrapVertex(int index, int lastVertex);
		int selectRefVertex(RdProcFace *pProcFace, int *pYMinVert, int *pYMaxVert);
		void selectMipLevel(RdActiveFace *pFace, int refVert);
		int getZShift(RdCanvas *pCanvas);
		void drawZSpans(RdActiveFace *pFace);
		void drawTexturedSpans(RdActiveFace *pFace);
		DrawSpanFunc selectWidthSpan(int texPitch, const DrawSpanFunc *apWidthSpans, DrawSpanFunc pfDefault);
		void setupFlat(RdActiveFace *pFace, RdMatCel *pCel, DrawFaceFunc pfDrawSpans, SetupEdgeFunc pfSetupEdge);
		void setupTextured(RdActiveFace *pFace, RdMatCel *pCel, bool bPersp, int lightKind, float light, DrawFaceFunc pfDrawSpans, SetupEdgeFunc pfSetupEdge);
		int setupEdgeAffine(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1, bool bTexture, bool bIntensity, StepEdgeFunc pfStep);
		void loadPerspState(RdActiveFace *pFace);
		void plotMaskedRun(SpanRun &run, int count, bool bpp16, int lightKind);
		void drawSpanMaskedAffine(RdActiveSpan *pSpan, bool bpp16, int lightKind);
		void drawSpanMaskedPersp(RdActiveSpan *pSpan, bool bpp16, int lightKind);
		void plotRun16(SpanRun &run, int count, bool bIntensity);
		void drawSpanAffine16(RdActiveSpan *pSpan, int uMask, int vShift, bool bIntensity);
		void drawSpanPersp16(RdActiveSpan *pSpan, int uMask, int vShift, bool bIntensity);

		int _mipMapNum;
		int _texRowShift;
		int _texPitch;
		int _uWrap;
		int _vWrap;
		int _dUdXModifier;
		int _dVdXModifier;
		int _dOneOverZdXFixed;
		float _dOneOverZdX;
		float _dUOverZdX;
		float _dVOverZdX;
		float _oneOverZStep;
		float _uOverZStep;
		float _vOverZStep;
		byte *_pPixels;
		byte *_aMonoMap;
		byte *_aRGBMap;
		byte _color;
		uint16 _RGBcolor;
		DrawSpanFunc _pfDrawSpan;
	};

	class NRasterModule : public Module {
	public:
		explicit NRasterModule(RenderDroid *rd);

		void calcAffineGradients(float z);
		void calcPerspGradients(float z);

		int drawNGonLV(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLW(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLS(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTLS(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonFS(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonTFS(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonGS(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTGS(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMTLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonMFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonMTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMTGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMTLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonMFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonMTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		int drawNGonGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonTGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonMTGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonLS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonFS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonLS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTFS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonGS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonGS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGS_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGS_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTLAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTLAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTFAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTFAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTGAT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTGAT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTLIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTLIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTFIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTFIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonTGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTGIT_8(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		void drawScanLineNGonMTGIT_16(RdNRasterEdge *pLeft, RdNRasterEdge *pRight);
		int setupLeftEdgeNGonLS(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonLS(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonFS(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonFS(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonGS(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonGS(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonLAT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonLAT(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonFAT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonFAT(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonGAT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonGAT(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonLIT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonLIT(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonFIT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonFIT(RdNRasterEdge *pEdge);
		int setupLeftEdgeNGonGIT(RdNRasterEdge *pEdge);
		int setupRightEdgeNGonGIT(RdNRasterEdge *pEdge);

	private:
		friend class RenderDroid;

		enum Geometry {
			kGeometrySolid,
			kGeometryAffine,
			kGeometryPerspective
		};

		enum Lighting {
			kLightingNone,
			kLightingFlat,
			kLightingGouraud
		};

		enum Blend {
			kBlendOpaque,
			kBlendAlpha,
			kBlendMasked,
			kBlendMaskedAlpha
		};

		enum EdgeKind {
			kEdgeFlat,
			kEdgeGouraud,
			kEdgeAffine,
			kEdgeAffineFlat,
			kEdgeAffineGouraud,
			kEdgePerspective,
			kEdgePerspectiveGouraud
		};

		struct SpanPlot {
			Lighting lighting;
			Blend blend;
			int bpp;
			bool perspective;
			byte *pDest8;
			uint16 *pDest16;
			const byte *pBackdrop;
		};

		struct SpanRun {
			int u;
			int v;
			int uStep;
			int vStep;
			int intensity;
			int dIntensity;
		};

		int ftol(double value);
		int floorToInt(double value);
		int fixedMultiply(int a, int b);
		int ceilToPixel(int x);
		int wrapVertex(int index);
		float lowRound(double value);
		float lowAdd(float a, float b);
		float lowSub(float a, float b);
		float lowMul(float a, float b);
		float lowDiv(float a, float b);

		int selectGradientVertex();
		void selectMipLevel(float z);

		int setupEdge(RdNRasterEdge *pEdge, int step, EdgeKind kind);
		EdgeKind getEdgeKind(Geometry geometry, Lighting lighting, bool isLeft);
		void advanceEdge(RdNRasterEdge *pEdge, Geometry geometry, Lighting lighting, bool isLeft);

		void setupFace(RdProcFace *pProcFace);
		void setupSolidColors(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, Lighting lighting, Blend blend);
		bool setupTexture(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, Geometry geometry, Lighting lighting, Blend blend);
		int drawNGon(RdProcFace *pProcFace, RdMatCel *pCel, float light, Geometry geometry, Lighting lighting, Blend blend);

		void drawScanLine(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Geometry geometry, Lighting lighting, Blend blend, int bpp);
		void drawScanLineSolid(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp);
		void drawScanLineAffine(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp);
		void drawScanLinePerspective(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, Lighting lighting, Blend blend, int bpp);
		int calcSpan(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, int *pXStart);
		void calcSpanIntensity(RdNRasterEdge *pLeft, RdNRasterEdge *pRight, int width, int subPixel, int *pIntensity, int *pDIntensity);
		bool beginSpanPlot(SpanPlot *pPlot, int y, int xStart, Lighting lighting, Blend blend, int bpp, bool perspective, bool bufferedDestination);
		void plotPixel(SpanPlot *pPlot, byte texel, int intensity);
		byte lightTexel(Lighting lighting, byte texel, int intensity);
		byte fetchTexel(int u, int v);
		byte fetchTexelByRows(int u, int v);
		void startRun(SpanRun *pRun, float u, float v, float uStepFixed, float vStepFixed);
		void plotRun(SpanPlot *pPlot, SpanRun *pRun, int count);

		uint _numVertices;
		int _yMinVert;
		int _yMaxVert;
		int _leftVertexStep;
		int _rightVertexStep;
		float _aOneOverZ[32];
		float _aUOverZ[32];
		float _aVOverZ[32];
		RdVector3 *_aVertices;
		RdVector2 *_aTexVertices;
		float *_aIntensities;
		byte *_pDest;
		uint16 *_pWDest;
		byte *_pPixels;
		int _mipMapNum;
		int _maxMipMap;
		int _texPitch;
		int _texRowShift;
		int _uWrap;
		int _vWrap;
		int _uStep;
		int _vStep;
		int _fixU;
		int _fixV;
		float _dOneOverZdX;
		float _dOneOverZdY;
		float _dUOverZdX;
		float _dUOverZdY;
		float _dVOverZdX;
		float _dVOverZdY;
		int _dUdXModifier;
		int _dVdXModifier;
		float _oneOverZStep;
		float _uOverZStep;
		float _vOverZStep;
		byte *_aAlphaMap;
		byte *_aMonoMap;
		uint16 *_aRGBMap;
		byte *_aRGBToPalette;
		uint16 _RGBcolor;
		byte _color;
	};

	class ZRasterModule : public Module {
	public:
		explicit ZRasterModule(RenderDroid *rd);
		void calcAffineGradients(float z);
		void calcPerspGradients(float z);
		int drawNGonLV(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLW(RdProcFace *pProcFace, RdMatCel *pCel);
		int drawNGonLS(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonLS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonLS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonLS(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonLS(RdZRasterEdge *pEdge);
		int setupLeftEdgeNGonFS(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonFS(RdZRasterEdge *pEdge);
		int drawNGonTLS(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTLS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTLS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonFS(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonFS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonFS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTFS(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonTFS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTFS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonGS(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonGS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonGS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonGS(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonGS(RdZRasterEdge *pEdge);
		int drawNGonTGS(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTGS_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTGS_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTLAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMTLAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTLAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonLAT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonLAT(RdZRasterEdge *pEdge);
		int drawNGonFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonTFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonMFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTFAT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonMTFAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTFAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonFAT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonFAT(RdZRasterEdge *pEdge);
		int drawNGonGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTGAT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMTGAT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTGAT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonGAT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonGAT(RdZRasterEdge *pEdge);
		int drawNGonLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTLIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMTLIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTLIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonLIT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonLIT(RdZRasterEdge *pEdge);
		int drawNGonFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonTFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonMFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTFIT(RdProcFace *pProcFace, RdMatCel *pCel, float light);
		void drawScanLineNGonMTFIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTFIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonFIT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonFIT(RdZRasterEdge *pEdge);
		int drawNGonGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonTGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonTGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonTGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int drawNGonMTGIT(RdProcFace *pProcFace, RdMatCel *pCel);
		void drawScanLineNGonMTGIT_8(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawScanLineNGonMTGIT_16(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		int setupLeftEdgeNGonGIT(RdZRasterEdge *pEdge);
		int setupRightEdgeNGonGIT(RdZRasterEdge *pEdge);
		void drawNGonLAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFAT_Case7(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGAT_Case7(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonLIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonFIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case0(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case1(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case2(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case3(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case4(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case5(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);
		void drawNGonGIT_Case6(RdZRasterEdge *pLeft, RdZRasterEdge *pRight);

	private:
		friend class RenderDroid;

		enum Mapping {
			kSolid,
			kAffine,
			kPerspective
		};

		enum Shading {
			kUnlit,
			kFlat,
			kGouraud
		};

		enum {
			kTransparent = 1,
			kMasked = 2
		};

		struct PixelMode {
			int bpp;
			int shading;
			int flags;
			bool perspective;
		};

		struct ScanParams {
			PixelMode mode;
			int uMask;
			int vShift;
			int lightMask;
			bool caseMode;
		};

		struct ScanState {
			int xLeft;
			int xStart;
			int x;
			int numPixels;
			byte *pDest;
			uint16 *pZ;
			int z;
			int zStep;
			int intensity;
			int intensityStep;
			int u;
			int v;
			int uStep;
			int vStep;
			bool bDither;
			int ditherX;
			int ditherY;
		};

		bool setZScaleShift();
		float truncToFloat(double value);
		int toFixed(double value);
		int wrapVertex(int index, int last);
		void selectGradientVertices(int *pVertA, int *pVertB, int *pRefVert);
		void selectMipLevel(float z);
		int gradientModifier(double dX, double dY);
		bool findEdgeSegment(RdZRasterEdge *pEdge, int vertexStep, uint *pVertIdx, uint *pNextIdx, float *pYStart, int *pNumLines);
		int interpolateEdgeValue(float from, float to, float invDy, float yOffset, int *pStep);
		int setupEdgeLinear(RdZRasterEdge *pEdge, int vertexStep, bool textured, bool gouraud);
		int setupLeftEdgePerspective(RdZRasterEdge *pEdge, bool gouraud);
		int setupRightEdgePerspective(RdZRasterEdge *pEdge, bool gouraud);
		int setupLeftEdge(RdZRasterEdge *pEdge, int mapping, int shading);
		int setupRightEdge(RdZRasterEdge *pEdge, int mapping, int shading);
		void stepEdge(RdZRasterEdge *pEdge, int mapping, int shading, bool isLeft);
		bool beginScanLine(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int bpp, ScanState *pState);
		void setupLinearZ(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, ScanState *pState);
		void setupLinearIntensity(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, ScanState *pState);
		int lightIndex(int index, bool gouraud);
		int readRGB(int index);
		bool plotTexel(const PixelMode &mode, byte *pDest, byte texel, int lightBase);
		void drawTextureRun(int count, ScanState *pState, const ScanParams &params);
		void scanLineSolid(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int bpp, int shading, int flags);
		ScanParams makeScanParams(int bpp, int shading, int flags, bool perspective);
		void scanLineAffine(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, const ScanParams &params);
		void setRunSteps(ScanState *pState, float uEnd, float vEnd, float uStart, float vStart, double stepScale);
		void scanLinePerspective(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, const ScanParams &params);
		void scanLineCase(RdZRasterEdge *pLeft, RdZRasterEdge *pRight, int mapping, int shading, int pitch);
		bool setupGeometry(RdProcFace *pProcFace);
		bool setupSolidFace(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, int shading, int flags);
		bool setupTexturedFace(RdProcFace *pProcFace, RdMatCel *pCel, float light, int bpp, int mapping, int shading, int flags);
		int selectCasePitch(int mapping, int shading);
		int drawNGon(RdProcFace *pProcFace, RdMatCel *pCel, float light, int mapping, int shading, int flags);

		uint _numVertices;
		int _maxMipMap;
		int _yMinVert;
		int _yMaxVert;
		int _leftVertexStep;
		int _rightVertexStep;
		float *_aIntensities;
		RdVector3 *_aVertices;
		RdVector2 *_aTexVertices;
		byte *_aRGBToPalette;
		byte *_aRGBMap;
		byte *_aMonoMap;
		byte *_aAlphaMap;
		byte *_pPixels;
		int _uWrap;
		int _vWrap;
		int _mipMapNum;
		int _texPitch;
		int _texRowShift;
		int _zScaleShift;
		int _dUdXModifier;
		int _dVdXModifier;
		int _dOneOverZdXFixed;
		float _oneOverZStep;
		float _uOverZStep;
		float _vOverZStep;
		float _dOneOverZdX;
		float _dOneOverZdY;
		float _dUOverZdX;
		float _dUOverZdY;
		float _dVOverZdX;
		float _dVOverZdY;
		uint16 _RGBcolor;
		byte _color;
		int _caseU;
		int _caseV;
		int _caseUStep;
		int _caseVStep;
		float _aOneOverZ[32];
		float _aUOverZ[32];
		float _aVOverZ[32];
	};

	CameraModule _camera;
	CanvasModule _canvas;
	ClipModule _clip;
	ColormapModule _colormap;
	ColorEffectsModule _colorEffects;
	FaceModule _face;
	KeyframeModule _keyframe;
	LightModule _light;
	MaterialModule _material;
	Model3Module _model3;
	Primit2Module _primit2;
	Primit3Module _primit3;
	PuppetModule _puppet;
	SpriteModule _sprite;
	ThingModule _thing;
	ActiveModule _active;
	CacheModule _cache;
	RasterModule _raster;
	AFRasterModule _afRaster;
	NRasterModule _nRaster;
	ZRasterModule _zRaster;

private:
	StdMath *_stdMath;
	StdDisplay *_stdDisplay;
	StdConffile *_stdConffile;
	StdFile *_stdFile;
	SysStruct *_pHS;
	int _bStarted;
	int _bOpened;
	int _frameTrue;
	int _curAcceleration;
	int _curRenderOptions;
	int _curGeometryMode;
	int _curLightingMode;
	int _curTextureMode;
	int _curSortingMethod;
	int _curOcclusionMethod;
	int _curZBufferMethod;
	int _curVertexColorMode;
	int _curCullFlags;
	int _curProcFaceUserData;
	StdColorFormat _colorInfo;
	RdColorEffects _curColorEffects;
	float _aMipDistances[4];
	RdMatrix34 _identMatrix34;
};

typedef void (RenderDroid::CameraModule::*RdProjectFunc)(RdVector3 pOut, RdVector3 pIn);
typedef void (RenderDroid::CameraModule::*RdProjectLstFunc)(RdVector3 *aOut, RdVector3 *aIn, int num);

typedef void (RenderDroid::AFRasterModule::*RdActiveEdgeStepFunc)(RdActiveEdge *pEdge);
typedef int (RenderDroid::AFRasterModule::*RdActiveSetupEdgeFunc)(RdActiveEdge *pEdge, RdActiveFace *pFace, int v0, int v1);
typedef void (RenderDroid::AFRasterModule::*RdActiveSpanFunc)(RdActiveFace *pFace);

struct RdActiveEdge {
	int bLeft;
	RdActiveFace *pFace;
	int yStart;
	int numLines;
	float oneOverZ;
	float dOneOverZ;
	float uOverZ;
	float dUOverZ;
	float vOverZ;
	float dVOverZ;
	int x;
	int dx;
	int u;
	int du;
	int v;
	int dv;
	int intensity;
	int dIntensity;
	RdActiveEdgeStepFunc pfStep;
	RdActiveEdge *pPrev;
	RdActiveEdge *pNext;
	RdActiveEdge *pNextRemove;
};

struct RdActiveSpan {
	int x;
	int width;
	int y;
	union {
		int u;
		float uOverZ;
	};
	union {
		int du;
		float vOverZ;
	};
	int v;
	int dv;
	float oneOverZ;
	int intensity;
	int dIntensity;
	RdActiveSpan *pNext;
};

struct RdActiveFace {
	RdProcFace *pProcFace;
	float aOneOverZ[32];
	float aUOverZ[32];
	float aVOverZ[32];
	float dOneOverZdX;
	float dOneOverZdY;
	float dUOverZdX;
	float dUOverZdY;
	float dVOverZdX;
	float dVOverZdY;
	int dOneOverZdXFixed;
	int uModifier;
	int vModifier;
	float dOneOverZdX16;
	float dUOverZdX16;
	float dVOverZdX16;
	RdActiveSpanFunc pfDrawSpans;
	RdActiveSpan *pFirstSpan;
	RdActiveSpan *pLastSpan;
	int mipLevel;
	byte *pTexPixels;
	int texShift;
	int texPitch;
	int colorIndex;
	int uWrap;
	int vWrap;
	byte *pLightLevel;
	byte *pRGB16;
	int unk_484;
	RdActiveSetupEdgeFunc pfSetupEdge;
	RdActiveEdge *pLeftEdge;
	RdActiveEdge *pRightEdge;
	int curX;
	int activeEdgeCount;
	RdActiveFace *pPrevFace;
	RdActiveFace *pNextFace;
};

struct RdCamera {
	int projectType;
	RdCanvas *pCanvas;
	RdMatrix34 viewMatrix;
	float fov;
	float focalLen;
	float aspectRatio;
	float orthoScale;
	RdClipFrustum *pClipFrustum;
	RdProjectFunc pfProject;
	RdProjectLstFunc pfProjectLst;
	float ambientLight;
	uint numLights;
	RdLight *apLights[64];
	RdVector3 aLightPos[64];
	float attenuationMin;
	float attenuationMax;
};

} // End of namespace Grim

#endif
