#pragma once

///////////////////////////////////////////////////////////////////////////////
// Menu commands for GUI only

// Menu entries for the application system menu -> require a set of IDs with special restrictions!
#define MP_RESTORE				0x110
#define MP_CONNECT				0x120
#define MP_DISCONNECT			0x130
#define MP_EXIT					0x140
#define	MP_ABOUTBOX				0x150
#define MP_VERSIONCHECK			0x160

#define MP_MESSAGE				10102
#define MP_DETAIL				10103
#define MP_ADDFRIEND			10104
#define MP_REMOVEFRIEND			10105
#define MP_SHOWLIST				10106
#define MP_FRIENDSLOT			10107
//MORPH START - Added by SiRoB, Friend Addon
#define MP_REMOVEALLFRIENDSLOT	10108
//MORPH END   - Added by SiRoB, Friend Addon

//MORPH START - Added by SiRoB, ZZ Upload System
#define MP_POWERSHARE_OFF       10161
#define MP_POWERSHARE_ON        10162
#define MP_POWERSHARE_AUTO      10163
//MORPH END   - Added by SiRoB, ZZ Upload System
//MORPH START - Added by SiRoB, Keep Permission flag
#define MP_PERMDEFAULT			10165
#define MP_PERMALL				10166
#define MP_PERMFRIENDS			10167
#define MP_PERMNONE				10168
// Mighty Knife: Community visible filelist
#define MP_PERMCOMMUNITY		10169
// [end] Mighty Knife
//MORPH END   - Added by SiRoB, Keep Permission flag
//MORPH START - Added by SiRoB, HIDEOS
#define MP_HIDEOS               10170
#define MP_HIDEOS_0             10171
#define MP_HIDEOS_1             10172
#define MP_HIDEOS_2             10173
#define MP_HIDEOS_3             10174
#define MP_HIDEOS_4             10175
#define MP_HIDEOS_5             10176
#define MP_SELECTIVE_CHUNK      10180
#define MP_SELECTIVE_CHUNK_0    10181
#define MP_SELECTIVE_CHUNK_1    10182
//MORPH END   - Added by SiRoB, HIDEOS
//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
#define MP_SHAREONLYTHENEED     10190
#define MP_SHAREONLYTHENEED_0   10191
#define MP_SHAREONLYTHENEED_1   10192
//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
//MORPH START - Added by IceCream, copy feedback feature
#define	MP_COPYFEEDBACK			10197
#define MP_COPYFEEDBACK_US		10198
//MORPH END   - Added by IceCream, copy feedback feature
#define MP_LIST_REQUESTED_FILES	10199	//MORPH START - Added by Yun.SF3, List Requested Files
#define MP_FORCE				10200	//shadow#(onlydownloadcompletefiles) //EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
#define MP_CANCEL				10201
#define MP_STOP					10202
#define MP_PAUSE				10203
#define MP_RESUME				10204
#define	MP_CLEARCOMPLETED		10205
#define	MP_OPEN					10206
#define	MP_PREVIEW				10207
#define MP_CMT					10208
#define MP_HM_CON				10209
#define MP_HM_SRVR				10210
#define MP_HM_TRANSFER			10211
#define MP_HM_SEARCH			10212
#define MP_HM_FILES				10213
#define MP_HM_MSGS				10214
#define MP_HM_IRC				10215
#define MP_HM_STATS				10216
#define MP_HM_PREFS				10217
#define MP_HM_OPENINC			10218
#define MP_HM_EXIT				10219
#define MP_ALL_A4AF_TO_THIS		10220
#define MP_ALL_A4AF_TO_OTHER	10221
#define MP_ALL_A4AF_AUTO		10222
#define MP_SWAP_A4AF_TO_THIS	10223
#define MP_SWAP_A4AF_TO_OTHER	10224
#define MP_META_DATA			10225
#define MP_BOOT					10226
#define MP_HM_CONVERTPF			10227
#define MP_RESUMEPAUSED			10228
#define MP_HM_KAD				10229
#define MP_HM_HELP				10242
#define MP_HM_1STSWIZARD		10243
#define MP_OPENFOLDER			10244
#define	MP_HM_IPFILTER			10245

#define MP_HM_LINK1				10230
#define MP_HM_LINK2				10231
#define MP_HM_LINK3				10232
#define MP_HM_SCHEDONOFF		10233

#define MP_SELECTTOOLBARBITMAPDIR 10234
#define MP_SELECTTOOLBARBITMAP	10235
#define MP_NOTEXTLABELS			10236
#define MP_TEXTLABELS			10237
#define MP_TEXTLABELSONRIGHT	10238
#define	MP_CUSTOMIZETOOLBAR		10239
#define	MP_SELECT_SKIN_FILE		10240
#define	MP_SELECT_SKIN_DIR		10241

#define MP_PRIOVERYLOW			10300
#define MP_PRIOLOW				10301
#define MP_PRIONORMAL			10302
#define MP_PRIOHIGH				10303
#define MP_PRIOVERYHIGH			10304
#define MP_PRIOAUTO				10317
#define MP_GETED2KLINK			10305
#define MP_GETHTMLED2KLINK		10306
#define	MP_GETSOURCEED2KLINK	10299
#define MP_METINFO				10307
#define MP_CONNECTTO			10311
#define MP_REMOVE				10312
#define MP_REMOVEALL			10313
#define MP_REMOVESELECTED		10314
#define MP_UNBAN				10315
#define MP_ADDTOSTATIC			10316
#define MP_CLCOMMAND			10317
#define MP_REMOVEFROMSTATIC		10318
#define MP_VIEWFILECOMMENTS		10319
#define MP_CAT_ADD				10321
#define MP_CAT_EDIT				10322
#define MP_CAT_REMOVE			10323
#define MP_SAVELOG				10324
#define MPG_DELETE				10325
#define	MP_COPYSELECTED			10326
#define	MP_SELECTALL			10327
#define	MP_AUTOSCROLL			10328
#define MP_RESUMENEXT			10329
#define MPG_ALTENTER			10330
#define MPG_F2					10331
#define	MP_RENAME				10332
#define	MP_FIND					10333

// khaos::categorymod+
#define MP_CAT_SHOWHIDEPAUSED	10335
#define MP_CAT_SETRESUMEORDER	10336
#define	MP_CAT_ORDERAUTOINC		10337
#define MP_CAT_ORDERSTEPTHRU	10338
#define MP_CAT_ORDERALLSAME		10339
#define MP_CAT_RESUMENEXT		10340
#define	MP_CAT_PAUSELAST		10341
#define MP_CAT_STOPLAST			10342
#define MP_CAT_MERGE			10343
#define MP_SETFILEGROUP			10344

// For the downloadlist menu
#define MP_FORCEA4AF			10350
#define	MP_FORCEA4AFONFLAG		10351
#define MP_FORCEA4AFOFFFLAG		10352

// 10353 to 10355 reserved for A4AF menu items.
#define MP_CAT_A4AF			10353
// khaos::categorymod-

#define MP_GETHOSTNAMESOURCEED2KLINK	10361	// itsonlyme: hostnameSource

// Mighty Knife: CRC32-Tag
#define MP_CALCCRC32            10380
#define MP_ADDCRC32TOFILENAME   10381
#define MP_ABORTCRC32CALC		10382
// [end] Mighty Knife

// Mighty Knife: Mass Rename
#define MP_MASSRENAME    		10385
// [end] Mighty Knife

// quick-speed changer
#define MP_QS_U10				10501
#define MP_QS_U20				10502
#define MP_QS_U30				10503
#define MP_QS_U40				10504
#define MP_QS_U50				10505
#define MP_QS_U60				10506
#define MP_QS_U70				10507
#define MP_QS_U80				10508
#define MP_QS_U90				10509
#define MP_QS_U100				10510
#define MP_QS_UPC				10511
#define MP_QS_UP10				10512
#define MP_QS_UPL				10513
#define MP_QS_D10				10521
#define MP_QS_D20				10522
#define MP_QS_D30				10523
#define MP_QS_D40				10524
#define MP_QS_D50				10525
#define MP_QS_D60				10526
#define MP_QS_D70				10527
#define MP_QS_D80				10528
#define MP_QS_D90				10529
#define MP_QS_D100				10530
#define MP_QS_DC				10531
#define MP_QS_DL				10532
#define MP_QS_PA				10533
#define MP_QS_UA				10534

#define MP_WEBURL				10600
// reserve some for weburls!
#define MP_ASSIGNCAT			10700
// reserve some for categories!
#define MP_SCHACTIONS			10800
// reserve some for schedules
#define MP_CAT_SET0				10900
// reserve some for change all-cats (about 20)
#define MP_TOOLBARBITMAP		10950
// reserve max 50
#define	MP_SKIN_PROFILE			11000
// reserve max 50


#define Irc_Version				"(SMIRCv00.67)"
#define Irc_Op					10240
#define Irc_DeOp				10241
#define Irc_Voice				10242
#define Irc_DeVoice				10243
#define Irc_HalfOp				10244
#define Irc_DeHalfOp			10245
#define Irc_Kick				10246
#define Irc_Slap				10247
#define Irc_Join				10248
#define Irc_Close				10249
#define Irc_Priv				10250
#define Irc_AddFriend			10251
#define	Irc_SendLink			10252
#define Irc_SetSendLink			10253
#define	Irc_Owner				10254
#define Irc_DeOwner				10255
#define	Irc_Protect				10256
#define Irc_DeProtect			10257
