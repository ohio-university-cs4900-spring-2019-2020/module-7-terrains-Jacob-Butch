#pragma once

#include "AftrAPI.h"
#include "IPhysicsManager.h"
#include "PxPhysicsAPI.h"

#ifdef AFTR_CONFIG_USE_BOOST

namespace Aftr {

	class WOCity : public WO {
		public:
			WOMacroDeclaration(WOCity, WO);

			static WOCity* New(
				const std::string& modelFileName = ManagerEnvironmentConfiguration::getSMM() + "/models/citytexday_3ds/city_tex_day.3DS",
				Vector scale = Vector(1, 1, 1),
				MESH_SHADING_TYPE shadingType = MESH_SHADING_TYPE::mstAUTO
			);
			virtual void onCreate(const std::string& modelFileName, Vector scale = Vector(1, 1, 1), MESH_SHADING_TYPE shadingType = MESH_SHADING_TYPE::mstAUTO);
			virtual void setPosition(Vector pos);
			virtual void setPosition(float x, float y, float z) { setPosition(Vector(x, y, z)); }

			physx::PxRigidStatic* actor;

		protected:
			float* vertexListCopy;
			unsigned int* indicesCopy;
			WOCity();
	};
}

#endif