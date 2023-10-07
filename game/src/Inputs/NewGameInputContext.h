#pragma once

#include "InputContext.h"


class NewGameInputContext final : public InputContext
{
public:
    inputs_t read_inputs(int controller_id) override;
};
