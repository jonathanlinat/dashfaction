#include "rf.h"
#include "spectate.h"
#include "config.h"
#include "utils.h"
#include "scoreboard.h"

#include "HookableFunPtr.h"
#include "hooks/HookCall.h"

//#define SPECTATE_SHOW_WEAPON_EXPERIMENTAL

#if SPECTATE_ENABLE

// FIXME: handle player left
static CPlayer *g_SpectateModeTarget;
static bool g_SpectateModeEnabled = false;
static int g_LargeFont = -1;

HookableFunPtr<0x004A35C0, void, CPlayer*> DestroyPlayerFun;
HookableFunPtr<0x004A6210, void, CPlayer *, EGameCtrl, char> HandleCtrlInGameFun;
HookCall<PFN_IS_PLAYER_ENTITY_INVALID> *pIsPlayerEntityInvalidCall;

static void SetCameraTarget(CPlayer *pPlayer)
{
    // Based on function SetCamera1View
    if (!*g_ppLocalPlayer || !(*g_ppLocalPlayer)->pCamera || !pPlayer)
        return;

    CAMERA *pCamera = (*g_ppLocalPlayer)->pCamera;
    pCamera->Type = CAMERA::RF_CAM_1ST_PERSON;
    pCamera->pPlayer = pPlayer;
    pPlayer->pCamera = pCamera; // fix crash 0040D744

    CEntity *pEntity = HandleToEntity(pPlayer->hEntity);
    if (pEntity)
    {
        CEntity *pCamEntity = pCamera->pEntity;
        pCamEntity->Head.hParent = pPlayer->hEntity;
        // pCamEntity->Head.hParent = pEntity->Head.Handle;
        pCamEntity->Head.vPos = pEntity->vWeaponPos;
        pCamEntity->Head.matRot = pEntity->Head.matRot;
        pCamEntity->matWeaponRot = pEntity->matWeaponRot;
        pCamEntity->Head.field_0 = pEntity->Head.field_0;
        pCamEntity->Head.field_4 = pEntity->Head.vPos;
    }
}

void SetSpectateModeTarget(CPlayer *pPlayer)
{
    if (!pPlayer)
        pPlayer = *g_ppLocalPlayer;

    if (!*g_ppLocalPlayer || !(*g_ppLocalPlayer)->pCamera)
        return;

    g_SpectateModeEnabled = (pPlayer != *g_ppLocalPlayer);
    g_SpectateModeTarget = pPlayer;

    SetScoreboardHidden(g_SpectateModeEnabled);
    KillLocalPlayer();
    SetCameraTarget(pPlayer);

#ifdef SPECTATE_SHOW_WEAPON_EXPERIMENTAL
    g_SpectateModeTarget = pPlayer;
    CEntity *pEntity = HandleToEntity(pPlayer->hEntity);
    SetupPlayerWeaponMesh(pPlayer, pEntity->WeaponSel.WeaponClsId);
    pPlayer->FireFlags |= 1 << 4;
    RfConsolePrintf("pWeaponMesh %p", pPlayer->pWeaponMesh);
#endif // SPECTATE_SHOW_WEAPON_EXPERIMENTAL
}

static void SpectateNextPlayer(bool bDir)
{
    CPlayer *pNewTarget = g_SpectateModeTarget;
    while (true)
    {
        pNewTarget = bDir ? pNewTarget->pNext : pNewTarget->pPrev;
        if (!pNewTarget || pNewTarget == g_SpectateModeTarget)
            return; // FIXME
        if (pNewTarget != *g_ppLocalPlayer)
            break;
    }

    SetSpectateModeTarget(pNewTarget);
}

static void HandleCtrlInGameHook(CPlayer *pPlayer, EGameCtrl KeyId, char WasPressed)
{
    if (g_SpectateModeEnabled)
    {
        if (KeyId == GC_PRIMARY_ATTACK)
        {
            if (WasPressed)
                SpectateNextPlayer(true);
            return; // dont allow spawn
        }
        else if (KeyId == GC_SECONDARY_ATTACK)
        {
            if (WasPressed)
                SpectateNextPlayer(false);
            return;
        }
    }
    HandleCtrlInGameFun.callOrig(pPlayer, KeyId, WasPressed);
}

static bool IsPlayerEntityInvalidHook(CPlayer *pPlayer)
{
    if (g_SpectateModeEnabled)
        return false;
    else
        return IsPlayerEntityInvalid(pPlayer);
}

void DestroyPlayerHook(CPlayer *pPlayer)
{
    if (g_SpectateModeTarget == pPlayer)
        SpectateNextPlayer(true);
    if (g_SpectateModeTarget == pPlayer)
        SetSpectateModeTarget(nullptr);
    DestroyPlayerFun.callOrig(pPlayer);
}

#ifdef SPECTATE_SHOW_WEAPON_EXPERIMENTAL

static void RenderPlayerArmHook(CPlayer *pPlayer)
{
    if (g_SpectateModeTarget)
        RenderPlayerArm(g_SpectateModeTarget);
    else
        RenderPlayerArm(pPlayer);
}

static void RenderPlayerArm2Hook(CPlayer *pPlayer)
{
    if (g_SpectateModeTarget)
        RenderPlayerArm2(g_SpectateModeTarget);
    else
        RenderPlayerArm2(pPlayer);
}

#endif // SPECTATE_SHOW_WEAPON_EXPERIMENTAL

void InitSpectateMode()
{
    pIsPlayerEntityInvalidCall = new HookCall<PFN_IS_PLAYER_ENTITY_INVALID>(0x00432A52, IsPlayerEntityInvalid);
    pIsPlayerEntityInvalidCall->Hook(IsPlayerEntityInvalidHook);

    HandleCtrlInGameFun.hook(HandleCtrlInGameHook);
    DestroyPlayerFun.hook(DestroyPlayerHook);

#ifdef SPECTATE_SHOW_WEAPON_EXPERIMENTAL
    //WriteMemPtr((PVOID)(0x0043285D + 1), (PVOID)((ULONG_PTR)RenderPlayerArmHook - (0x0043285D + 0x5)));
    WriteMemPtr((PVOID)(0x004A2B56 + 1), (PVOID)((ULONG_PTR)RenderPlayerArm2Hook - (0x004A2B56 + 0x5)));
    WriteMemUInt8Repeat((PVOID)0x004AB1B8, ASM_NOP, 6); // RenderPlayerArm2
    WriteMemUInt8Repeat((PVOID)0x004AA23E, ASM_NOP, 6); // SetupPlayerWeaponMesh
    WriteMemUInt8Repeat((PVOID)0x004AE0DF, ASM_NOP, 2); // PlayerLoadWeaponMesh

    WriteMemUInt8Repeat((PVOID)0x004AA3B1, ASM_NOP, 6); // sub_4AA3A0
    WriteMemUInt8((PVOID)0x004A952C, ASM_SHORT_JMP_REL); // sub_4A9520
    WriteMemUInt8Repeat((PVOID)0x004AA56D, ASM_NOP, 6); // sub_4AA560
    WriteMemUInt8Repeat((PVOID)0x004AE384, ASM_NOP, 6); // PlayerPrepareWeapon
#endif // SPECTATE_SHOW_WEAPON_EXPERIMENTAL
}

void DrawSpectateModeUI()
{
    if (!g_SpectateModeEnabled)
        return;

    if (g_LargeFont == -1)
        g_LargeFont = GrLoadFont("rfpc-large.vf", -1);

    char szBuf[256];
    const unsigned cx = 400, cy = 50;
    unsigned cxScr = RfGetWidth(), cySrc = RfGetHeight();
    unsigned x = (cxScr - cx) / 2;
    unsigned y = cySrc - 100;
    unsigned cyFont = GrGetFontHeight(-1);

    GrSetColorRgb(0, 0, 0x20, 0x60);
    GrDrawRect(x, y, cx, cy, *((uint32_t*)0x17756C0));

    GrSetColorRgb(0, 0xFF, 0, 0x80);
    sprintf(szBuf, "Spectating: %s", g_SpectateModeTarget->strName.psz);
    GrDrawAlignedText(1, x + cx / 2, y + cy / 2 - cyFont / 2, szBuf, -1, *g_pDrawTextUnk);

    CEntity *pEntity = HandleToEntity(g_SpectateModeTarget->hEntity);
    if (!pEntity)
    {
        GrSetColorRgb(0xC0, 0, 0, 0xC0);
        GrDrawAlignedText(1, cxScr / 2, cySrc / 2, "DEAD", g_LargeFont, *g_pDrawTextUnk);
    }
}

#endif