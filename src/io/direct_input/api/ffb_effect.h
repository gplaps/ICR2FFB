#pragma once
#include "project_dependencies.h"

#include <dinput.h>

#include <string>

struct FFBEffect
{
public:
    virtual ~FFBEffect();

    void         Start();
    virtual void Update(LONG magnitude_strength, bool withDirection = false) = 0;

protected:
    explicit FFBEffect(const std::wstring& typeName) :
        effect(NULL),
        started(false),
        effectName(typeName) {}

    IDirectInputEffect* effect;
    bool                started;
    std::wstring        effectName;
};

struct DiConstantEffect : public FFBEffect
{
    explicit DiConstantEffect(IDirectInputDevice8* device);
    void Update(LONG magnitude, bool withDirection) OVERRIDE;
};

struct DiDamperEffect : public FFBEffect
{
    explicit DiDamperEffect(IDirectInputDevice8* device);
    void Update(LONG magnitude, bool withDirection = false) OVERRIDE;
};

struct DiSpringEffect : public FFBEffect
{
    explicit DiSpringEffect(IDirectInputDevice8* device);
    void Update(LONG magnitude, bool withDirection = false) OVERRIDE;
};
