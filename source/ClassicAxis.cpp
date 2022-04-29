﻿#include "plugin.h"

#include "CPad.h"
#include "CGeneral.h"
#include "CCam.h"
#include "CCamera.h"
#include "CWorld.h"
#include "CSprite2d.h"
#include "CWeaponInfo.h"
#include "CMenuManager.h"
#include "CHud.h"
#include "CAnimBlendAssociation.h"
#include "CAnimManager.h"
#include "CSprite.h"
#include "CWeaponEffects.h"

#include "ClassicAxis.h"
#include "Utility.h"
#include "CamNew.h"

ClassicAxis classicAxis;

ClassicAxis::ClassicAxis() {
    classicAxis.settings.Read();

    plugin::Events::initRwEvent += [] {
        GInput_Load(&classicAxis.pXboxPad);

        classicAxis.Clear();
    };

#ifdef GTA3
    plugin::CdeclEvent < plugin::AddressList<0x48C7BE, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickNone, void()> onReinitGame;
#else
    plugin::CdeclEvent < plugin::AddressList<0x4A46F1, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickNone, void()> onReinitGame;
#endif

    onReinitGame.after += [] {
        classicAxis.Clear();
    };

    // Patches
    auto process_FollowPedWithMouse = [](CCam* cam, int, CVector const& target, float targetOrient, float, float) {
        if (!CamNew)
            return;

        CamNew->Process_FollowPed(target, targetOrient, 0.0f, 0.0f);
    };

#ifdef GTA3
    plugin::patch::RedirectCall(0x459A7C, (void(__fastcall*)(CCam*, int, CVector const&, float, float, float))process_FollowPedWithMouse);
    plugin::patch::Nop(0x459A65, 2);

#else
    plugin::patch::RedirectCall(0x483BA1, (void(__fastcall*)(CCam*, int, CVector const&, float, float, float))process_FollowPedWithMouse);
    plugin::patch::Nop(0x483B8A, 2);
#endif

    // Fps to 3rd - m16.
#ifdef GTA3
    plugin::patch::SetChar(0x4F1F72 + 2, -1);
#else
    plugin::patch::SetChar(0x5349DB, 0x90); // M4
    plugin::patch::SetChar(0x5349DB + 1, 0x90); // M4
    plugin::patch::SetChar(0x5349E0, 0x90); // Ruger
    plugin::patch::SetChar(0x5349E0 + 1, 0x90); // Ruger
    plugin::patch::SetChar(0x5349E5, 0x90); // M60
    plugin::patch::SetChar(0x5349E5 + 1, 0x90); // M60
#endif
    // No fight cam
#ifdef GTA3
    plugin::patch::Nop(0x469226, 9);
    plugin::patch::Nop(0x469260, 9);
#else
    // No fight cam
    plugin::patch::Nop(0x4715D9, 9);
    plugin::patch::Nop(0x471613, 9);

    // Fix jump fail
    plugin::patch::SetChar(0x4F0031, 0xEB);
#endif

    auto playerMovementType = [](int, int) {
        int moveType = TYPE_WALKAROUND;

        if (classicAxis.forceRealMoveAnim) {
            classicAxis.forceRealMoveAnim = false;
            return static_cast<int>(TYPE_WALKAROUND);
        }

        if (classicAxis.isAiming && !classicAxis.ignoreRotation)
            moveType = TYPE_STRAFE;

        return moveType;
    };

#ifdef GTA3
    int playerMovementTypeAddr[] = { 0x4F2685, 0x469292, 0x4C7673, 0x4F03D5, 0x4D76CE, 0x45DB98 };
#else
    int playerMovementTypeAddr[] = { 0x4720B1, 0x533BCE, 0x4711DB, 0x50A8E3, 0x537D1C, 0x4F002A };
#endif

    for (int i = 0; i < ARRAYSIZE(playerMovementTypeAddr); i++)
        plugin::patch::RedirectCall(playerMovementTypeAddr[i], (int(__fastcall*)(int, int))playerMovementType);

    auto playerShootingDirection = [](int, int) {
        int moveType = TYPE_WALKAROUND;

        if (classicAxis.isAiming && !CWorld::Players[CWorld::PlayerInFocus].m_pPed->m_bHasLockOnTarget)
            moveType = TYPE_STRAFE;

        return moveType;
    };

#ifdef GTA3
    int playerShootingDirectionAddr[] = { 0x4E6562, 0x55D88B, 0x560C25, 0x561E63, 0x4EDAD1 };
#else
    int playerShootingDirectionAddr[] = { 0x52D73E, 0x5D18E7, 0x5CDA44, 0x5CCC14 };
#endif
    for (int i = 0; i < ARRAYSIZE(playerShootingDirectionAddr); i++)
        plugin::patch::RedirectCall(playerShootingDirectionAddr[i], (int(__fastcall*)(int, int))playerShootingDirection);

    auto using3rd = [](CCam*, int) { return false; };
#ifdef GTA3
    plugin::patch::SetChar(0x468E83 + 3, 0);
    plugin::patch::RedirectCall(0x468E46, (bool(__fastcall*)(CCam*, int))using3rd);
#else
    plugin::patch::SetChar(0x471218 + 3, 0);
    plugin::patch::RedirectCall(0x4711DB, (bool(__fastcall*)(CCam*, int))using3rd);
#endif
#if GTA3
    auto clearWeaponTarget = [](CPlayerPed* playa, int) {
        if (!classicAxis.isAiming)
            playa->ClearWeaponTarget();
    };
    plugin::patch::RedirectCall(0x4F22A0, (void(__fastcall*)(CPlayerPed*, int))clearWeaponTarget);

    plugin::patch::Nop(0x4F21A0, 6);
    plugin::patch::Nop(0x4F21CA, 7);

    plugin::patch::Set<BYTE>(0x468D09, 0x74);
    plugin::patch::Set<BYTE>(0x468D92, 0x74);
#else
    plugin::patch::Nop(0x534D42, 6);
    plugin::patch::Set<BYTE>(0x534D7B, 0x74);

    plugin::patch::Set<BYTE>(0x471093, 0x74);
    plugin::patch::Set<BYTE>(0x47111B, 0x74);

    auto clearAimFlag = [](CPed* playa, int) {
        if (!classicAxis.isAiming)
            playa->ClearAimFlag();
    };
    plugin::patch::RedirectCall(0x52C274, (void(__fastcall*)(CPed*, int))clearAimFlag);
#endif

    // No point gun cam
#ifdef GTA3
    plugin::patch::Nop(0x469C14, 6);
    plugin::patch::Nop(0x469CDD, 9);
    plugin::patch::Nop(0x469D5A, 9);
#else
    plugin::patch::Nop(0x472422, 6);
    plugin::patch::Nop(0x4724D5, 9);
    plugin::patch::Nop(0x47254F, 9);
#endif

    // Transition duration
#ifdef GTA3
    plugin::ThiscallEvent <plugin::AddressList<0x46A9C3, plugin::H_CALL, 0x46AB94, plugin::H_CALL, 0x46AC11, plugin::H_CALL, 0x470D8C, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPick2N<CCam*, 0, short, 1>, void(CCam*, short)> onStartingTransition;
#else
    plugin::ThiscallEvent <plugin::AddressList<0x47360B, plugin::H_CALL, 0x4734E5, plugin::H_CALL, 0x473305, plugin::H_CALL, 0x46AB00, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPick2N<CCam*, 0, short, 1>, void(CCam*, short)> onStartingTransition;
#endif

    onStartingTransition.after += [](CCam* cam, short mode) {
        if (mode == MODE_AIMWEAPON || mode == 4) {
            if (classicAxis.switchTransitionSpeed) {
                const int transitionDuration = 550;

                TheCamera.m_nTransitionDuration = transitionDuration;
#ifdef GTAVC
                TheCamera.m_nTransitionDurationTargetCoors = transitionDuration;
#endif
                TheCamera.m_fFractionInterToStopMoving = 0.1f;
                TheCamera.m_fFractionInterToStopCatchUp = 0.9f;

                classicAxis.switchTransitionSpeed = false;
                classicAxis.camUseCurrentAngle = true;
            }
        }
    };

#ifdef GTA3
    plugin::ThiscallEvent <plugin::AddressList<0x46D5FF, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickN<CCam*, 0>, void(CCam*)> onProcessingCam;
#else
    plugin::ThiscallEvent <plugin::AddressList<0x46C7A3, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickN<CCam*, 0>, void(CCam*)> onProcessingCam;
#endif
    onProcessingCam.before += [](CCam* cam) {
        if (cam->m_nCamMode > LAST_CAM_MODE) {
            classicAxis.previousSource = cam->m_vecSource;
            classicAxis.previousFront = cam->m_vecFront;
            classicAxis.previousUp = cam->m_vecUp;
        }
    };

    onProcessingCam.after += [](CCam* cam) {
        if (cam->m_nCamMode > LAST_CAM_MODE) {
            cam->m_vecSource = classicAxis.previousSource;
            cam->m_vecFront = classicAxis.previousFront;
            cam->m_vecUp = classicAxis.previousUp;
        }

        CVector target = {};
        float targetOrient = 0.0f;
        CEntity* e = cam->m_pCamTargetEntity;

        if (!e)
            return;

        target = e->GetPosition();

#ifdef GTA3
        CMatrix& mat = e->m_matrix;
#else
        CMatrix& mat = e->m_placement;
#endif
        if (mat.at.x == 0.0f && mat.at.y == 0.0f)
            targetOrient = 0.0f;
        else
            targetOrient = CGeneral::GetATanOfXY(mat.at.x, mat.at.y);

        char mode = cam->m_nCamMode;
        switch (mode) {
        case MODE_AIMWEAPON:
            CamNew->Process_AimWeapon(target, targetOrient, 0.0f, 0.0f);
            break;
        }
    };

    auto camControl = [](int, int) {
        TheCamera.m_bUseMouse3rdPerson = true;
        FrontEndMenuManager.m_ControlMethod = 0;
        TheCamera.m_f3rdPersonCHairMultX = classicAxis.settings.cameraCrosshairMultX;
        TheCamera.m_f3rdPersonCHairMultY = classicAxis.settings.cameraCrosshairMultY;
#ifdef GTAVC
        plugin::patch::SetPointer(0x46F925 + 2, &classicAxis.settings.cameraCrosshairMultX);
#endif

        TheCamera.CamControl();
    };
#ifdef GTA3
    plugin::patch::RedirectCall(0x46D500, (void(__fastcall*)(int, int))camControl);
#else
    plugin::patch::RedirectCall(0x46C694, (void(__fastcall*)(int, int))camControl);
#endif

#ifdef GTA3
    plugin::ThiscallEvent <plugin::AddressList<0x4EFE50, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickN<CPed*, 0>, void(CPed*)> onProcessingPlayerControl;
#else
    plugin::ThiscallEvent <plugin::AddressList<0x53739F, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPickN<CPed*, 0>, void(CPed*)> onProcessingPlayerControl;
#endif
    onProcessingPlayerControl += [](CPed* ped) {
        CPlayerPed* playa = static_cast<CPlayerPed*>(ped);
        classicAxis.ProcessPlayerPedControl(playa);
        classicAxis.Find3rdPersonMouseTarget(playa);
    };

#ifdef GTA3
    plugin::patch::SetChar(0x50554C + 1, 0);
    plugin::patch::SetChar(0x505627 + 1, 0);
#else
    auto ss = [](int, int, int, int, int, int, int, int, int, int, int, int) {};
    plugin::patch::RedirectCall(0x557713, (void(__fastcall*)(int, int, int, int, int, int, int, int, int, int, int, int))ss);
    plugin::patch::RedirectCall(0x5577F7, (void(__fastcall*)(int, int, int, int, int, int, int, int, int, int, int, int))ss);
#endif
    plugin::Events::drawHudEvent += [] {
        classicAxis.DrawCrosshair();

        bool disableAutoAim = !classicAxis.pXboxPad->HasPadInHands() && !classicAxis.settings.forceAutoAim;
        if (!disableAutoAim)
            classicAxis.DrawAutoAimTarget();

        if (classicAxis.settings.showTriangleForMouseRecruit)
            classicAxis.DrawTriangleForMouseRecruitPed();
    };

    if (classicAxis.settings.lockOnTargetType > TARGET_DEFAULT) {
#ifdef GTA3
        plugin::patch::SetUChar(0x564D04 + 6, 0);
#else
        plugin::patch::SetUChar(0x5D5064 + 6, 0);
#endif
    }

    // Menu
#ifdef GTA3
    const CMenuScreen controllerSetup = {
        "FET_CTL", 41, 41, 41, 0, 0,
        MENUACTION_REDEFCTRL, "FET_RDK",  0, MENUPAGE_KEYBOARD_CONTROLS,
        MENUACTION_CHANGEMENU, "FET_AMS", 0, MENUPAGE_MOUSE_CONTROLS,
        MENUACTION_RESTOREDEF, "FET_DEF", 0, MENUPAGE_CONTROLLER_PC,
        MENUACTION_CHANGEMENU, "FEDS_TB", 0, MENUPAGE_NONE,
    };

    plugin::patch::Set(0x611930 + sizeof(CMenuScreen) * 35, controllerSetup);
#else
    const CMenuScreen controllerSetup = {
    "FET_CTL", MENUPAGE_OPTIONS, 0,
    {
        { MENUACTION_REDEFCTRL, "FEC_RED", 0, MENUPAGE_KEYBOARD_CONTROLS, 320, 150, 3 },
        { MENUACTION_CHANGEMENU,"FEC_MOU", 0, MENUPAGE_MOUSE_CONTROLS, 0, 0, 3 },
        { MENUACTION_RESTOREDEF, "FET_DEF", 0, MENUPAGE_CONTROLLER_PC, 0, 0, 3 },
        { MENUACTION_GOBACK, "FEDS_TB", 0, MENUPAGE_NONE, 0, 0, 3 },
    }
    };

    plugin::patch::Set(0x6D8B70 + sizeof(CMenuScreen) * MENUPAGE_CONTROLLER_PC, controllerSetup);
#endif

    // Weapon cycle
    auto processWeaponSwitch = [](CPlayerPed* ped, int, CPad* pad) {
        if (ped != FindPlayerPed())
            return;

        if (ped->m_ePedState == ePedState::PEDSTATE_AIMGUN || ped->m_ePedState == PEDSTATE_ATTACK)
            return;

        if (!classicAxis.isAiming)
            return ped->ProcessWeaponSwitch(pad);
    };

#ifdef GTA3
    plugin::patch::RedirectCall(0x4F0464, (void(__fastcall*)(CPlayerPed*, int, CPad*))processWeaponSwitch);
#else
    plugin::patch::RedirectCall(0x537DC9, (void(__fastcall*)(CPlayerPed*, int, CPad*))processWeaponSwitch);
#endif

    // Weapon smooth spray
    auto doWeaponSmoothSpray = [](CPlayerPed* ped, int) {
#ifdef GTA3
        return false;
#else
        if (ped->m_nPedFlags.bIsDucking)
            return 0.00001f;
        else
            return -1.0f;
#endif
    };
#ifdef GTA3
    plugin::patch::RedirectCall(0x4F13CC, (bool(__fastcall*)(CPlayerPed*, int))doWeaponSmoothSpray);
#else
    plugin::patch::RedirectCall(0x535F4C, (float(__fastcall*)(CPlayerPed*, int))doWeaponSmoothSpray);
#endif
   
    // Point gun while ducking fix
#ifdef GTAVC
    plugin::ThiscallEvent <plugin::AddressList<0x53491B, plugin::H_CALL, 0x535326, plugin::H_CALL, 0x5355C6, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPick3N<CPed*, 0, int, 1, __int16, 2>, void(CPed*, int, __int16)> onDucking;
    onDucking += [](CPed* ped, int, __int16) {
        if (ped != FindPlayerPed())
            return;

        const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

        if (!info->m_bCrouchFire && !classicAxis.IsTypeMelee(ped)) {
            ped->m_nPedFlags.bCrouchWhenShooting = false;
            ped->RestorePreviousState();
        }
    };

    plugin::ThiscallEvent <plugin::AddressList<0x534968, plugin::H_CALL, 0x535355, plugin::H_CALL, 0x5355F5, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPick2N<CPed*, 0, char, 1>, void(CPed*, char)> onClearDuck;
    onClearDuck += [](CPed* ped, char) {
        if (ped != FindPlayerPed())
            return;

        ped->ClearPointGunAt();
        classicAxis.wasCrouching = false;
    };

    // Duck while targetting fix
    plugin::ThiscallEvent <plugin::AddressList<0x534D64, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPickN<CPlayerPed*, 0>, void(CPlayerPed*)> onFindingNewLockOnTarget;
    onFindingNewLockOnTarget += [](CPlayerPed* ped) {
        if (ped != FindPlayerPed())
            return;

        const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

        if ((!ped->m_nPedFlags.bIsDucking || info->m_bCrouchFire)) {

        }
        else {
            ped->ClearPointGunAt();
            ped->ClearWeaponTarget();
        }
    };
#endif

    // Make 1st person cam mode point on the front
#ifdef GTA3
    plugin::ThiscallEvent <plugin::AddressList<0x4F1FDB, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPick4N<CCam*, 0, int, 1, int, 2, int, 3>, void(CCam*, int, int, int)> onSet1stPersonPlayerCamMode;
    plugin::patch::Nop(0x461B61, 10);
    plugin::patch::Nop(0x4624A2, 10);
#else
    plugin::ThiscallEvent <plugin::AddressList<0x534A60, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPick4N<CCam*, 0, int, 1, int, 2, int, 3>, void(CCam*, int, int, int)> onSet1stPersonPlayerCamMode;
    plugin::patch::Nop(0x47949C, 10);
    plugin::patch::Nop(0x47A289, 10);
#endif

    onSet1stPersonPlayerCamMode += [](CCam* cam, int, int, int) {
        if (TheCamera.m_asCams[TheCamera.m_nActiveCam].m_nCamMode == 4) {
            CPlayerPed* playa = FindPlayerPed();
            float angle = CGeneral::LimitRadianAngle(-TheCamera.m_fOrientation);
            classicAxis.RotatePlayer(playa, angle, false);
        }
    };
    
#ifdef GTA3
    plugin::ThiscallEvent <plugin::AddressList<0x4F21B7, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPickN<CPlayerPed*, 0>, void(CPlayerPed*)> onFindingLockOnTarget;
#else
    plugin::ThiscallEvent <plugin::AddressList<0x534D64, plugin::H_CALL>, plugin::PRIORITY_AFTER, plugin::ArgPickN<CPlayerPed*, 0>, void(CPlayerPed*)> onFindingLockOnTarget;
#endif
    onFindingLockOnTarget += [](CPlayerPed* ped) {
        bool disableAutoAim = !classicAxis.pXboxPad->HasPadInHands() && !classicAxis.settings.forceAutoAim;
        if (disableAutoAim || 
            (ped->m_pPointGunAt && !ped->m_pPointGunAt->GetIsOnScreenComplex()))
            ped->ClearWeaponTarget();
    };
}

void ClassicAxis::RotatePlayer(CPed* ped, float angle, bool smooth) {
    if (classicAxis.ignoreRotation)
        return;

    if (smooth) {
        ped->m_fRotationDest = angle;
        ped->m_fRotationCur = CGeneral::LimitRadianAngle(ped->m_fRotationCur);
        float angle = ped->m_fRotationDest;

        if (ped->m_fRotationCur - M_PI > ped->m_fRotationDest) {
            angle += 2 * M_PI;
        }
        else if (M_PI + ped->m_fRotationCur < ped->m_fRotationDest) {
            angle -= 2 * M_PI;
        }

        ped->m_fRotationCur += (angle - ped->m_fRotationCur) * 0.02f;
    }
    else {
        ped->m_fRotationCur = angle;
        ped->m_fRotationDest = angle;
#ifdef GTA3
        ped->m_matrix.SetRotateZOnly(ped->m_fRotationCur);
#else
        ped->m_placement.SetRotateZOnly(ped->m_fRotationCur);
#endif
    }
}

void ClassicAxis::Clear() {
    isAiming = false;
    wasPointing = false;
    wasCrouching = false;
    ignoreRotation = false;
    forceRealMoveAnim = false;
    lastLockOnPos = {};
    timeLockOn = 0;
    lastLockOnColor = { 255, 255, 255, 255 };
    thirdPersonMouseTarget = NULL;

    switchTransitionSpeed = false;

    if (!CamNew)
        CamNew = std::make_shared<CCamNew>();

    previousSource = CamNew->cam->m_vecSource;
    previousFront = CamNew->cam->m_vecFront;
    previousUp = CamNew->cam->m_vecUp;
    previousCamMode = CamNew->cam->m_nCamMode;
    previousHorAngle = CamNew->cam->m_fHorizontalAngle;
    previousVerAngle = CamNew->cam->m_fVerticalAngle;
    camUseCurrentAngle = false;
}

bool ClassicAxis::IsAbleToAim(CPed* ped) {
    ePedState s = ped->m_ePedState;
    eMoveState m = ped->m_eMoveState;
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    switch (s) {
    case PEDSTATE_NONE:
    case PEDSTATE_IDLE:
    case PEDSTATE_FLEE_POSITION:
    case PEDSTATE_FLEE_ENTITY:
    case PEDSTATE_ATTACK:
    case PEDSTATE_FIGHT:
    case PEDSTATE_AIMGUN:
        return m != PEDMOVE_SPRINT && ped->IsPedInControl()
#ifdef GTAVC
            && (!ped->m_nPedFlags.bIsDucking || info->m_bCrouchFire)
#endif
            ;
    default:
        return false;
    }
}

bool ClassicAxis::IsType1stPerson(CPed* ped) {
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    return !info->m_bCanAim && !info->m_bCanAimWithArm && info->m_b1stPerson;
}

bool ClassicAxis::IsWeaponPossiblyCompatible(CPed* ped) {
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    switch (weaponType) {
    case WEAPONTYPE_FLAMETHROWER:
#ifndef GTA3
    case WEAPONTYPE_MINIGUN:
#endif
        info->m_bCanAim = true;
        break;
    }

    return (info->m_bCanAim || info->m_bCanAimWithArm) && !info->m_bThrow && !info->m_b1stPerson;
}

bool ClassicAxis::IsTypeMelee(CPed* ped) {
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;

#ifdef GTA3
    switch (weaponType) {
    case WEAPONTYPE_UNARMED:
    case WEAPONTYPE_BASEBALLBAT:
    case WEAPONTYPE_MOLOTOV:
    case WEAPONTYPE_GRENADE:
    case WEAPONTYPE_DETONATOR:
        return true;
    }
#else
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);
    return !info->m_bCanAim && !info->m_bCanAimWithArm;
#endif

    return false;
}

bool ClassicAxis::IsTypeTwoHanded(CPed* ped) {
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    switch (weaponType) {
    case WEAPONTYPE_FLAMETHROWER:
#ifndef GTA3
    case WEAPONTYPE_MINIGUN:
#endif
        info->m_bCanAim = true;
        break;
    }

    return (info->m_bCanAim || info->m_bHeavy) && !info->m_bCanAimWithArm && !info->m_b1stPerson;
}

void ClassicAxis::DrawCrosshair() {
    CCam cam = TheCamera.m_asCams[TheCamera.m_nActiveCam];
    CPlayerPed* playa = FindPlayerPed();
    CPad* pad = CPad::GetPad(0);

    if (cam.m_nCamMode != MODE_AIMWEAPON)
        return;

    if (TheCamera.m_nTransitionState != 0)
        return;

    if (!playa->m_nFlags.bIsVisible)
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void*>(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void*>(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void*>(FALSE));

    if (playa) {
        const eWeaponType weaponType = playa->m_aWeapons[playa->m_nCurrentWeapon].m_eWeaponType;
        char mode = cam.m_nCamMode;
        float x = RsGlobal.maximumWidth * TheCamera.m_f3rdPersonCHairMultX;
        float y = RsGlobal.maximumHeight * TheCamera.m_f3rdPersonCHairMultY;
        y -= ScaleY(2.0f);
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);
        eWeaponState state = playa->m_aWeapons[playa->m_nCurrentWeapon].m_eWeaponState;

        CSprite2d* crosshair = &CHud::Sprites[HUD_SITEM16];
        if (crosshair) {
            if (classicAxis.isAiming) {
                if (!playa->m_bInVehicle && (mode == 4 || mode == MODE_AIMWEAPON) && !pad->DisablePlayerControls) {
                    if (!playa->m_bHasLockOnTarget && classicAxis.IsWeaponPossiblyCompatible(playa))
                        crosshair->Draw(CRect(x - ScaleX(14.0f), y - ScaleY(14.0f), x + ScaleX(14.0f), y + ScaleY(14.0f)), CRGBA(255, 255, 255, 255));
                }

            }
        }
    }
}

void ClassicAxis::DrawAutoAimTarget() {
    if (settings.lockOnTargetType == TARGET_DEFAULT)
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void*>(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void*>(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void*>(FALSE));

    CPlayerPed* playa = FindPlayerPed();

    if (playa) {
        CEntity* e = playa->m_pPointGunAt;
        CRGBA col = { 0, 255, 0, 255 };
        int targetMode = 0;

        if (e) {
            lastLockOnPos.x = e->GetPosition().x;
            lastLockOnPos.y = e->GetPosition().y;
            lastLockOnPos.z = e->GetPosition().z;

            if (e->m_nType == ENTITY_TYPE_PED) {
                CPed* ep = static_cast<CPed*>(e);

                if (ep) {
                    ep->m_PedIK.GetComponentPosition(lastLockOnPos, 1);

                    float health = ep->m_fHealth / 100.0f;

                    if (settings.lockOnTargetType == TARGET_SA)
                        col = CRGBA((1.0f - health) * 255, health * 255, 0, 255);
                    else
                        col = CRGBA(0, health * 255, 0, 255);

                    if (health <= 0.0f)
                        col = CRGBA(0, 0, 0, 255);
                }
            }

            lastLockOnPos.z += 0.25f;

            if (playa->m_bHasLockOnTarget && e) {
                targetMode = 1;
                timeLockOn = 250 + CTimer::m_snTimeInMilliseconds;
                lastLockOnColor = col;
            }
        }

        RwV3d in = lastLockOnPos;
        RwV3d out;
        float w, h;
        float rotMult = 0.5f;
        float dist = 1.0f;

        if (timeLockOn > CTimer::m_snTimeInMilliseconds) {
            if (CSprite::CalcScreenCoors(in, &out, &w, &h, false)) {
                switch (settings.lockOnTargetType) {
                case TARGET_SA:
                    dist = (w / 128.0f) * static_cast<float>((timeLockOn / (250 + CTimer::m_snTimeInMilliseconds)));
                    if (targetMode == 0) {
                        rotMult = 3.0f;
                    }
                    DrawSATarget(out.x, out.y, dist, rotMult, lastLockOnColor);
                    break;
                case TARGET_LCS:
                    dist = w / 128.0f;
                    col.a = 150;
                    if (targetMode == 1)
                        DrawLCSTarget(out.x, out.y, dist, col);
                    break;
                }
            }
        }
        else {
            lastLockOnColor = col;
        }
    }
}

void ClassicAxis::DrawTriangleForMouseRecruitPed() {
    if (TheCamera.m_nTransitionState != 0)
        return;

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void*>(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void*>(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void*>(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void*>(FALSE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void*>(FALSE));

    CPlayerPed* playa = FindPlayerPed();

    if (playa) {
        if (thirdPersonMouseTarget) {
            RwV3d in;
            RwV3d out;
            float w, h;

            in.x = thirdPersonMouseTarget->GetPosition().x;
            in.y = thirdPersonMouseTarget->GetPosition().y;
            in.z = thirdPersonMouseTarget->GetPosition().z;
            thirdPersonMouseTarget->m_PedIK.GetComponentPosition(in, 1);
            in.z += 1.0f;

            float health = thirdPersonMouseTarget->m_fHealth / 100.0f;
            CRGBA col = CRGBA((1.0f - health) * 255, health * 255, 0, 150);

            if (health <= 0.0f)
                col = CRGBA(0, 0, 0, 255);

            if (CSprite::CalcScreenCoors(in, &out, &w, &h, false)) {
                DrawSATriangleForMouseRecruit(out.x, out.y, w / 128.0f, col);
            }
        }
    }
}

void ClassicAxis::ClearWeaponTarget(CPlayerPed* ped) {
    ped->m_bHasLockOnTarget = false;
    ped->m_pPointGunAt = NULL;
    TheCamera.ClearPlayerWeaponMode();
    gCrossHair.ClearCrossHair();
}

void ClassicAxis::ProcessPlayerPedControl(CPed* ped) {
    CPlayerPed* playa = FindPlayerPed();
    if (!playa)
        return;

    if (playa != ped)
        return;

    CPad* pad = CPad::GetPad(0);
    CCam& cam = TheCamera.m_asCams[TheCamera.m_nActiveCam];
    short& mode = cam.m_nCamMode;
    float front = CGeneral::LimitRadianAngle(-TheCamera.m_fOrientation);
    float height = Find3rdPersonQuickAimPitch(TheCamera.m_f3rdPersonCHairMultY);
    CWeapon& currentWeapon = playa->m_aWeapons[playa->m_nCurrentWeapon];
    eWeaponType weaponType = currentWeapon.m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    if (pad->NewKeyState.lmenu && settings.altKeyWalk) {
        playa->m_fMoveSpeed = 0.0f;
    }

    isAiming = false;
    ignoreRotation = false;

    if (!pad->DisablePlayerControls && IsAbleToAim(playa) && pad->GetTarget() && (TheCamera.GetLookDirection() != 0) && IsWeaponPossiblyCompatible(playa) && !IsType1stPerson(playa) && (mode == 4 || mode == MODE_AIMWEAPON)) {
        isAiming = true;
        if (mode != MODE_AIMWEAPON && playa->IsPedInControl() && TheCamera.m_nTransitionState == 0) {
            TheCamera.TakeControl(FindPlayerPed(), MODE_AIMWEAPON, 1, 0);
            TheCamera.m_bLookingAtPlayer = false;
            classicAxis.switchTransitionSpeed = true;
            classicAxis.previousHorAngle = cam.m_fHorizontalAngle;
            classicAxis.previousVerAngle = cam.m_fVerticalAngle;

            previousCamMode = mode;
        }

        CEntity* p = playa->m_pPointGunAt;
        float mouseX = pad->NewMouseControllerState.X;
        float mouseY = pad->NewMouseControllerState.Y;
        bool disableAutoAim = !pXboxPad->HasPadInHands() && !settings.forceAutoAim;

        if (!disableAutoAim) {
            if (playa->m_bHasLockOnTarget && p) {
                CVector diff = p->GetPosition() - playa->GetPosition();
                front = CGeneral::GetATanOfXY(diff.x, diff.y) - M_PI_2;

                RwV3d out;
                RwV3d in;
                in.x = p->GetPosition().x;
                in.y = p->GetPosition().y;
                in.z = p->GetPosition().z;

                if (p->m_nType == ENTITY_TYPE_PED) {
                    CPed* ep = static_cast<CPed*>(p);
                    ep->m_PedIK.GetComponentPosition(in, 1);
                    in.z += 0.25f;
                }

                if (CSprite::CalcScreenCoors(in, &out, &in.x, &in.y, false)) {
                    height = Find3rdPersonQuickAimPitch(out.y / SCREEN_HEIGHT);
                }

                bool transitionDone = true;
                if (TheCamera.m_nTransitionState != 0)
                    transitionDone = false;

                if ((transitionDone && ((pXboxPad->HasPadInHands() &&
                    (pad->Mode == 4 ? pad->NewState.LeftShoulder2 < 99 : (pad->LookAroundLeftRight() || pad->LookAroundUpDown()))) || (abs(mouseX) > 1.0f || abs(mouseY) > 1.0f))) ||
                    diff.Magnitude() < 0.5f)
                    ClearWeaponTarget(playa);
            }
            else {
                if (/*!pad->SecondaryFireJustDown() && */(pad->Mode == 4 ? pad->NewState.LeftShoulder2 >= 100 : pad->ShiftTargetLeftJustDown() || pad->ShiftTargetRightJustDown()))
                    playa->FindWeaponLockOnTarget();
            }
        }

        RotatePlayer(playa, front, false);
#ifdef GTA3
        playa->SetLookFlag(front, true);
#else
        playa->SetLookFlag(front, false, false);
#endif
        playa->SetAimFlag(front);

        playa->m_fFPSMoveHeading = height;
        playa->m_fFPSMoveHeading = clamp(playa->m_fFPSMoveHeading, -DegToRad(45.0f), DegToRad(45.0f));

        float torsoPitch = 0.0f;
        if (!info->m_bCanAimWithArm || playa->m_nPedFlags.bIsDucking) {
            torsoPitch = playa->m_fFPSMoveHeading;
        }

        if (playa->m_vecMoveSpeed.Magnitude() < 0.01f)
            forceRealMoveAnim = true;

        playa->m_PedIK.MoveLimb(playa->m_PedIK.m_sHead, playa->m_PedIK.m_sHead.m_fYaw, 0.0f, playa->m_PedIK.ms_headInfo);
        playa->m_PedIK.MoveLimb(playa->m_PedIK.m_sTorso, 0.0f, torsoPitch, playa->m_PedIK.ms_torsoInfo);
        playa->m_PedIK.MoveLimb(playa->m_PedIK.m_sLowerArm, 0.0f, playa->m_fFPSMoveHeading, playa->m_PedIK.ms_lowerArmInfo);

#ifdef GTA3
        CAnimBlendAssociation* anim = plugin::CallAndReturn<CAnimBlendAssociation*, 0x4055C0>(playa->m_pRwClump, info->m_AnimToPlay);
#else
        CAnimBlendAssociation* anim = RpAnimBlendClumpGetAssociation(playa->m_pRwClump, 205);
#endif
        bool point = true;

        if (anim && anim->m_fCurrentTime < info->m_fAnimLoopEnd) {
            point = false;
        }

        if (pad->GetWeapon())
            point = false;

        if (currentWeapon.m_eWeaponState == WEAPONSTATE_OUT_OF_AMMO || currentWeapon.m_eWeaponState == WEAPONSTATE_RELOADING)
            point = false;

        if (!currentWeapon.HasWeaponAmmoToBeUsed()) {
            playa->ProcessWeaponSwitch(pad);
            point = false;
        }

        if (point)
            playa->SetPointGunAt(NULL);

        wasPointing = true;

#ifdef GTAVC
        if (playa->m_nPedFlags.bIsDucking && !wasCrouching) {
            playa->m_ePedState = PEDSTATE_FIGHT;
            wasCrouching = true;
        }
#endif
    }

    if (!isAiming) {
        if (wasPointing) {
            if (playa->m_ePedState != PEDSTATE_ATTACK && playa->m_nPedFlags.bIsAimingGun) {
                playa->ClearPointGunAt();
                playa->ClearWeaponTarget();
            }

#ifdef GTAVC
            if (wasCrouching) {
                if (currentWeapon.m_eWeaponState != WEAPONSTATE_OUT_OF_AMMO && currentWeapon.m_eWeaponState != WEAPONSTATE_RELOADING &&
                    IsAbleToAim(playa)) {
                    CAnimManager::BlendAnimation(playa->m_pRwClump, 0, 159, 4.0f);
                    playa->SetDuck(60000, 1);
                }
                wasCrouching = false;
            }
#endif				
            if (mode != previousCamMode && TheCamera.m_nTransitionState == 0) {
                TheCamera.TakeControl(FindPlayerPed(), previousCamMode, 1, 0);
                TheCamera.m_bLookingAtPlayer = true;
                classicAxis.switchTransitionSpeed = true;
                classicAxis.previousHorAngle = cam.m_fHorizontalAngle;
                classicAxis.previousVerAngle = cam.m_fVerticalAngle;
                previousCamMode = mode;
                wasPointing = false;
            }

        }
    }

    if (playa->m_ePedState == PEDSTATE_ATTACK && IsAbleToAim(playa) && ((IsTypeTwoHanded(playa) && !IsTypeMelee(playa) && mode == 4))) {
        RotatePlayer(playa, front, true);
    }
}

float ClassicAxis::Find3rdPersonQuickAimPitch(float y) {
    CCam* cam = &TheCamera.m_asCams[TheCamera.m_nActiveCam];
    CVector front = cam->m_vecFront;
    float fov = cam->m_fFOV;
    return -(DegToRad(((0.5f - y) * 1.8f * 0.5f * fov)) + asinf(front.z));
}

void ClassicAxis::Find3rdPersonMouseTarget(CPlayerPed* ped) {
    if (classicAxis.pXboxPad->HasPadInHands())
        return;

    CCam* cam = &TheCamera.m_asCams[TheCamera.m_nActiveCam];

    CVector source;
    CVector target;
    CEntity* e = NULL;
    CColPoint point = {};
    const eWeaponType weaponType = ped->m_aWeapons[ped->m_nCurrentWeapon].m_eWeaponType;
    CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(weaponType);

    if (isAiming && !ped->m_bInVehicle && (!ped->m_bHasLockOnTarget || !ped->m_pPointGunAt)) {
        TheCamera.Find3rdPersonCamTargetVector(info->m_fRange, cam->m_vecSource, source, target);
        if (CWorld::ProcessLineOfSight(source, target, point, e, false, false, true, false, false, false, false
#ifdef GTAVC
            , false
#endif
        )) {
            if (e->m_nType == ENTITY_TYPE_PED) {
                CPed* target = static_cast<CPed*>(e);

                if (target && target != ped && target->m_ePedState != PEDSTATE_DEAD) {
                    if (!thirdPersonMouseTarget || thirdPersonMouseTarget != target) {
                        thirdPersonMouseTarget = target;
                        thirdPersonMouseTarget->RegisterReference(&e);
                        thirdPersonMouseTarget->ReactToPointGun(ped);
#ifdef GTAVC
                        ped->Say(117);
#endif
                    }
                }
            }
            return;
        }
    }

    if (thirdPersonMouseTarget) {
#ifdef GTAVC
        e = static_cast<CEntity*>(thirdPersonMouseTarget);
        thirdPersonMouseTarget->CleanUpOldReference(&e);
#endif
        thirdPersonMouseTarget = NULL;
    }
}

