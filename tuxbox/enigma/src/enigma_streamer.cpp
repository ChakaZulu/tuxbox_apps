#include <enigma_streamer.h>

eStreamer *eStreamer::instance;

eStreamer::eStreamer()
{
	instance = this;
}

eStreamer::~eStreamer()
{
}

void eStreamer::setServiceReference(eServiceReference psref)
{
	sref = psref;
}

bool eStreamer::getServiceReference(eServiceReference& psref)
{
	psref = sref;
	return !(system("pidof streamts"));
}

