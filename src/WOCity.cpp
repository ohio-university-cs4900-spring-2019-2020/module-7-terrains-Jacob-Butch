#include "WOCity.h"

#ifdef AFTR_CONFIG_USE_BOOST

using namespace Aftr;
using namespace physx;

WOCity* WOCity::New(const std::string& modelFileName, Vector scale, MESH_SHADING_TYPE shadingType) {
	WOCity* city = new WOCity();
	city->onCreate(modelFileName, scale, shadingType);
	return city;
}

WOCity::WOCity() : IFace(this), WO() {
	this->vertexListCopy = nullptr;
	this->indicesCopy = nullptr;
}

void WOCity::onCreate(const std::string& modelFileName, Vector scale, MESH_SHADING_TYPE shadingType) {
	WO::onCreate(modelFileName, scale, shadingType);
	this->renderOrderType = RENDER_ORDER_TYPE::roOVERLAY;
	this->setLabel("City");

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
	meshDesc.points.stride = PxU32(sizeof(float) * 3);
	meshDesc.points.data = this->vertexListCopy;

	meshDesc.triangles.count = PxU32(indexListSize / 3);
	meshDesc.triangles.stride = PxU32(3 * sizeof(unsigned int));
	meshDesc.triangles.data = this->indicesCopy;
	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = IPhysicsManager::getCooking()->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status) {
		std::cout << "Failed to create Triangular mesh" << std::endl;
		std::cin.get();
	}
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	physx::PxTriangleMesh* mesh = IPhysicsManager::physics->createTriangleMesh(readBuffer);

	PxMaterial* gMaterial = IPhysicsManager::createMaterial(0.5f, 0.5f, 0.6f);
	PxShape* shape = IPhysicsManager::createShape(PxTriangleMeshGeometry(mesh), *gMaterial, true);

	this->actor = IPhysicsManager::createRigidStatic(PxTransform({0, 0, 0}));
	bool b = this->actor->attachShape(*shape);

	this->actor->userData = this;
	IPhysicsManager::addActor(this->actor);
	this->setPosition(Vector(0.0f, 0.0f, 1355.0f));
}


void WOCity::setPosition(Vector pos) {
	WO::setPosition(pos);
	if (this->actor != nullptr) {
		PxMat44 mat = IPhysicsManager::mat4ToMat44(this->getDisplayMatrix());
		mat.setPosition(IPhysicsManager::toPxVec3(pos));
		this->actor->setGlobalPose(PxTransform(mat));
	}
}

#endif