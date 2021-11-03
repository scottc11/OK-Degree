#include "GlobalControl.h"

using namespace DEGREE;

void GlobalControl::init() {
    _degrees->init();

    _channels[0]->init();
    _channels[1]->init();
    _channels[2]->init();
    _channels[3]->init();

    _degrees->attachCallback(callback(this, &GlobalControl::handleDegreeChange));
}

void GlobalControl::poll()
{
    _degrees->poll();
    _channels[0]->poll();
    _channels[1]->poll();
    _channels[2]->poll();
    _channels[3]->poll();
}

void GlobalControl::handleDegreeChange()
{
    _channels[0]->updateDegrees();
    _channels[1]->updateDegrees();
    _channels[2]->updateDegrees();
    _channels[3]->updateDegrees();
}