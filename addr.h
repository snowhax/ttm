#ifndef __ADDR_H__
#define __ADDR_H__

/* mem addr */
#define MemAddrStaticData                             0x1D615F8   //0x1C76820
#define MemAddrUserObject                             0x1D65E20   //0x1C79C30
#define MemAddrMobileInventory                        0x1D660CC   //0x1C79EDC
#define MemAddrBuffCardSystem                         0x1D615E4   //0x1C7680C
#define MemAddrBuffManager                            0x1D66588   //0x1C7A3BC
#define MemAddrStatManager                            0x1D660C4   //0x1C79ED4
#define MemAddrCharacterInfo                          0x1D615B4   //0x1C767DC
#define MemAddrSpellManager                           0x1D662F4   //0x1C7A134
#define MemAddrSelectedObjectManager                  0x1D66008   //0x1C79E18
#define MemAddrLayoutManager                          0x1D61620   //0x1C76848
#define MemAddrDirector                               0x1DD70F0   //0x1CE00D0
#define MemAddrGameLoopSystem                         0x1D61628   //0x1C76850
#define MemAddrMapSystem                              0x1D65D68   //0x1C79B78
#define MemAddrActor                                  0x1D48214   //0x1C5D2C0
#define MemAddrClientDataManager                      0x1D615B0   //0x1C767D8
#define MemAddrAutoSelector                           0x1D66004   //0x1C79E14
#define MemAddrCharacterPlayManager                   0x1D65FF0   //0x1C79E00
#define MemAddrPlayerInGameData                       0x1D6661C   //0x1C7A464

/* resource key ptr */
#define KeyPtrDescData                                0x1D662CC   //0x1C7A100
#define KeyPtrItemClass                               0x1D65D5C   //0x1C79B6C
#define KeyPtrSpellClass                              0x1D65D60   //0x1C79B70
#define KeyPtrSpellLearnLimit                         0x1D66100   //0x1C79F10
#define KeyPtrBuffCard                                0x1D66A28   //0x1C7A818


/* fun addr */
#define FunAddrMainloop                               (0x1521214)   //0x14893B0
#define FunAddrUseItem                                (0x53BE7C|1)  //0x5326B8
#define FunAddrUseItemLimit                           (0x000000|1)  //
#define FunAddrUIClick                                (0x138C9D4)   //0x12F4AEC
#define FunAddrSelect                                 (0x56E964|1)  //0x564D94
#define FunAddrUseSpell                               (0x5AB838|1)  //0x5A0C2C
#define FunAddrChatLog                                (0x556754|1)  //0x54C9DC
#define FunAddrSetAuto                                (0x5035D8|1)  //0x4F073C
#define FunAddrSetSelf                                (0x56FA14|1)  //0x565E4C
#define FunAddrGetMapArea                             (0x536C18|1)  //0x52A50C
#define FunAddrGetMapWorld                            (0x536FC0|1)  //0x52A8B4
#define FunAddrMapIsSafe                              (0x70DD94|1)  //0x6F24FC
#define FunAddrMapIsFight                             (0x70DDFC|1)  //0x6F2564
#define FunAddrShowCampaign                           (0xC1D78C|1)  //0xBEFED0

#endif
