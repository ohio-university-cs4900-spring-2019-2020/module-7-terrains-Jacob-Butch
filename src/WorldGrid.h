#pragma once

#include <iostream>
#include "AftrAPI.h"
#include "IPhysicsManager.h"

#ifdef AFTR_CONFIG_USE_BOOST

namespace Aftr {
    class WO;

	class WorldGrid : public WOGridECEFElevation {
        public:
            WOMacroDeclaration(WorldGrid, WOGridECEFElevation);
            static WorldGrid* New(Vector& centerOfWorld, Vector& gravityDirection, int splits = 2, float exageration = 0, bool useColors = false);

            virtual void setPosition(Vector pos);
            virtual void setPosition(float x, float y, float z) { setPosition(Vector(x, y, z)); }

            void setDisplayMatrix(Mat4 matrix) { WO::getModel()->setDisplayMatrix(matrix); }	
            void toPhysx();

            physx::PxRigidStatic* actor;

        protected:
            virtual void onCreate(Vector& centerOfWorld, Vector& gravityDirection, int splits, float exageration, bool useColors);
            virtual ~WorldGrid() {};
            WorldGrid();

            float* vertexListCopy;
            unsigned int* indicesCopy;
	};
}

#endif