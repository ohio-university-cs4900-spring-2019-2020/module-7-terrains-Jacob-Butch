#include "WorldGrid.h"

#include <iostream>

#ifdef AFTR_CONFIG_USE_BOOST

using namespace Aftr;
using namespace physx;

WOMacroDeclaration(WorldGrid, WOGridECEFElevation);

WorldGrid* WorldGrid::New(Vector& centerOfWorld, Vector& gravityDirection, int splits, float exageration, bool useColors) {
	WorldGrid* grid = new WorldGrid();
	grid->onCreate(centerOfWorld, gravityDirection, splits, exageration, useColors);
    return grid;
}

WorldGrid::WorldGrid() : IFace(this), WOGridECEFElevation() {
	this->actor = nullptr;
	this->vertexListCopy = nullptr;
	this->indicesCopy = nullptr;
}

void WorldGrid::onCreate(Vector& centerOfWorld, Vector& gravityDirection, int splits, float exageration, bool useColors) {
    float top = 34.2072593790098f;
    float bottom = 33.9980272592999f;

    float left = -118.65234375f;
    float right = -118.443603515625f;

    float vert = top - bottom;
    float horz = right - left;

    Vector offset(((top + bottom) / 2.0f), ((left + right) / 2.0f), 0.0f);
    centerOfWorld = offset.toVecS().toECEFfromWGS84();
    gravityDirection = centerOfWorld;
    gravityDirection.normalize();

    const int gran = 50;
    std::vector<Vector> inner;
    std::vector<std::vector<Vector>> outer;
    inner.resize(gran);
    for (int i = 0; i < gran; i++) {
        outer.push_back(inner);
    }
    for (int i = 0; i < gran; i++) {
        for (int j = 0; j < gran; j++) {
            outer[i][j] = Vector(bottom + ((float) i / gran) * vert, left + ((float) j / gran) * horz, 0.0f);
        }
    }

    VectorD scale = VectorD(1.0, 1.0, 1.0);
    VectorD upperLeft((double) top, (double) left, 0.0);
    VectorD lowerRight((double) bottom, (double) right, 0.0);

    WOGridECEFElevation::onCreate(upperLeft, lowerRight, 0, offset, scale, "../mm/terrains/Woodland.tif", splits, exageration, useColors);
    //WOGridECEFElevation::onCreate(upperLeft, lowerRight, 0, offset, scale, "../mm/terrains/terrain1.tif", splits, exageration, useColors);
    this->setLabel("World Grid");

    ///*
    VectorD ll = ((upperLeft + lowerRight) / 2.0);
    ll.z = 0.0;
    VectorD zdir = ll.toECEFfromWGS84().normalizeMe();
    VectorD northPoleECEF = VectorD(90.0, 0.0, 0.0).toECEFfromWGS84();
    VectorD xdir = northPoleECEF - ll.toECEFfromWGS84();
    xdir = xdir.vectorProjectOnToPlane(zdir);
    xdir.normalize();
    VectorD ydir = zdir.crossProduct(xdir);
    ydir.normalize();

    //create the Local Tangent Plane (LTP) transformation matrix
    float localBodySpaceToLTP[16] = {
        (float) xdir.x, (float) xdir.y, (float) xdir.z, 0.0f,
        (float) ydir.x, (float) ydir.y, (float) ydir.z, 0.0f,
        (float) zdir.x, (float) zdir.y, (float) zdir.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    this->setDisplayMatrix(Mat4(localBodySpaceToLTP).transposeUpperLeft3x3());
    //*/
    Texture* texture = ManagerTexture::loadTexture("../mm/terrains/woodland.bmp");
    //Texture* texture = ManagerTexture::loadTexture("../mm/terrains/terrain1.bmp");
    size_t pos = 0;
    for (ModelMesh* mesh : getModel()->getModelDataShared()->getModelMeshes()) {
        mesh->getSkin().getMultiTextureSet().at(pos) = texture;
    }
    
    this->getModel()->isUsingBlending(false);
    //this->getModel()->setNormalDirection(Vector(0.83f, 0.0f, 0.558f));
    //this->getModel()->setLookDirection(Vector(0.267f, -0.878f, -0.397f));
    this->toPhysx();
    this->setPosition(0.0f, 0.0f, -1000.0f);
}

void WorldGrid::setPosition(Vector pos) {
    WO::setPosition(pos);
    if (this->actor != nullptr) {
        PxMat44 mat = IPhysicsManager::mat4ToMat44(this->getDisplayMatrix());
        mat.setPosition(IPhysicsManager::toPxVec3(pos));
        this->actor->setGlobalPose(PxTransform(mat));
    }
}

void WorldGrid::toPhysx() {
	ModelDataShared* modelData = this->getModel()->getModelDataShared();
	//make copy of vertex list and index list
	size_t vertexListSize = modelData->getCompositeVertexList().size();
	size_t indexListSize = modelData->getCompositeIndexList().size();

	this->vertexListCopy = new float[vertexListSize * 3];
	this->indicesCopy = new unsigned int[indexListSize];

	for (size_t i = 0; i < vertexListSize; i++) {
		this->vertexListCopy[i * 3 + 0] = modelData->getCompositeVertexList().at(i).x;
		this->vertexListCopy[i * 3 + 1] = modelData->getCompositeVertexList().at(i).y;
		this->vertexListCopy[i * 3 + 2] = modelData->getCompositeVertexList().at(i).z;
	}
	for (size_t i = 0; i < indexListSize; i++) {
		this->indicesCopy[i] = modelData->getCompositeIndexList().at(i);
	}

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = PxU32(vertexListSize);
	meshDesc.points.stride = sizeof(float) * 3;
	meshDesc.points.data = this->vertexListCopy;

	meshDesc.triangles.count = PxU32(indexListSize / 3);
	meshDesc.triangles.stride = 3 * sizeof(unsigned int);
	meshDesc.triangles.data = this->indicesCopy;
	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = IPhysicsManager::getCooking()->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status) {
		std::cout << "Failed to create Triangular mesh" << std::endl;
		std::cin.get();
	}
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	physx::PxTriangleMesh* mesh = IPhysicsManager::createTriangleMesh(readBuffer);

	PxMaterial* gMaterial = IPhysicsManager::createMaterial(0.5f, 0.5f, 0.6f);
	PxShape* shape = IPhysicsManager::createShape(PxTriangleMeshGeometry(mesh), *gMaterial, true);

	this->actor = IPhysicsManager::createRigidStatic(PxTransform(PxVec3(0.0f, 0.0f, 0.0f)));
	bool b = this->actor->attachShape(*shape);

	this->actor->userData = this;
	IPhysicsManager::addActor(this->actor);
}
#endif