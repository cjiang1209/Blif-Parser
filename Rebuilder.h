#ifndef REBUILDER_H_
#define REBUILDER_H_

#include <meddly.h>

#include "ModelBuilder.h"

class Rebuilder : public ModelBuilder
{
public:
	Rebuilder(const Model& model);
	virtual void reorder(int* order);
};

#endif
