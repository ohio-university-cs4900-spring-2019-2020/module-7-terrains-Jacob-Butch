#include "NetMsgToilet.h"

#include <iostream>
#include <sstream>
#include <string>

#ifdef AFTR_CONFIG_USE_BOOST

using namespace Aftr;

NetMsgMacroDefinition(NetMsgToilet);

bool NetMsgToilet::toStream(NetMessengerStreamBuffer& os) const {
	os << pos.x << pos.y << pos.z;
	return true;
}

bool NetMsgToilet::fromStream(NetMessengerStreamBuffer& is) {
	float x, y, z;
	is >> x >> y >> z;
	this->pos = Vector(x, y, z);
	return true;
}

void NetMsgToilet::onMessageArrived() {
	((GLViewModule*) ManagerGLView::getGLView())->makeToilet(this->pos);
}

std::string NetMsgToilet::toString() const {
	std::stringstream ss;
	ss << NetMsg::toString();
	ss << "   Message: " << this->pos << std::endl;

	return ss.str();
}

#endif