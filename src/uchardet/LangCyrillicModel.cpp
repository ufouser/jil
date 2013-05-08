/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSBCharSetProber.h"



//KOI8-R language model
//Character Mapping Table:
static const unsigned char KOI8R_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,  //80
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,  //90
223,224,225, 68,226,227,228,229,230,231,232,233,234,235,236,237,  //a0
238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,  //b0
 27,  3, 21, 28, 13,  2, 39, 19, 26,  4, 23, 11,  8, 12,  5,  1,  //c0
 15, 16,  9,  7,  6, 14, 24, 10, 17, 18, 20, 25, 30, 29, 22, 54,  //d0
 59, 37, 44, 58, 41, 48, 53, 46, 55, 42, 60, 36, 49, 38, 31, 34,  //e0
 35, 43, 45, 32, 40, 52, 56, 33, 61, 62, 51, 57, 47, 63, 50, 70,  //f0
};

static const unsigned char win1251_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,
223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
239,240,241,242,243,244,245,246, 68,247,248,249,250,251,252,253,
 37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
 45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
  3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
  9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
};

static const unsigned char latin5_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,
223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
 37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
 45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
  3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
  9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
239, 68,240,241,242,243,244,245,246,247,248,249,250,251,252,255,
};

static const unsigned char macCyrillic_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
 37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
 45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,
223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
239,240,241,242,243,244,245,246,247,248,249,250,251,252, 68, 16,
  3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
  9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27,255,
};

static const unsigned char IBM855_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
191,192,193,194, 68,195,196,197,198,199,200,201,202,203,204,205,
206,207,208,209,210,211,212,213,214,215,216,217, 27, 59, 54, 70,
  3, 37, 21, 44, 28, 58, 13, 41,  2, 48, 39, 53, 19, 46,218,219,
220,221,222,223,224, 26, 55,  4, 42,225,226,227,228, 23, 60,229,
230,231,232,233,234,235, 11, 36,236,237,238,239,240,241,242,243,
  8, 49, 12, 38,  5, 31,  1, 34, 15,244,245,246,247, 35, 16,248,
 43,  9, 45,  7, 32,  6, 40, 14, 52, 24, 56, 10, 33, 17, 61,249,
250, 18, 62, 20, 51, 25, 57, 30, 47, 29, 63, 22, 50,251,252,255,
};

static const unsigned char IBM866_CharToOrderMap[] =
{
255,255,255,255,255,255,255,255,255,255,254,255,255,254,255,255,  //00
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  //10
+253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,253,  //20
252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,  //30
253,142,143,144,145,146,147,148,149,150,151,152, 74,153, 75,154,  //40
155,156,157,158,159,160,161,162,163,164,165,253,253,253,253,253,  //50
253, 71,172, 66,173, 65,174, 76,175, 64,176,177, 77, 72,178, 69,  //60
 67,179, 78, 73,180,181, 79,182,183,184,185,253,253,253,253,253,  //70
 37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
 45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
  3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,
207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,
223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
  9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
239, 68,240,241,242,243,244,245,246,247,248,249,250,251,252,255,
};

//Model Table: 
//total sequences: 100%
//first 512 sequences: 97.6601%
//first 1024 sequences: 2.3389%
//rest  sequences:      0.1237%
//negative sequences:   0.0009% 
static const PRUint8 RussianLangModel[] = 
{
0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,3,3,3,3,1,3,3,3,2,3,2,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,3,2,2,2,2,2,0,0,2,
3,3,3,2,3,3,3,3,3,3,3,3,3,3,2,3,3,0,0,3,3,3,3,3,3,3,3,3,2,3,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,2,2,3,3,3,3,3,3,3,3,3,2,3,3,0,0,3,3,3,3,3,3,3,3,2,3,3,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,2,3,2,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,3,3,3,3,3,3,3,3,3,3,3,2,1,
0,0,0,0,0,0,0,2,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,0,0,3,3,3,3,3,3,3,3,3,3,3,2,1,
0,0,0,0,0,1,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,2,2,2,3,1,3,3,1,3,3,3,3,2,2,3,0,2,2,2,3,3,2,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,3,3,3,3,3,2,2,3,2,3,3,3,2,1,2,2,0,1,2,2,2,2,2,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,3,0,2,2,3,3,2,1,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,1,0,0,2,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,3,3,1,2,3,2,2,3,2,3,3,3,3,2,2,3,0,3,2,2,3,1,1,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,2,2,3,3,3,3,3,2,3,3,3,3,2,2,2,0,3,3,3,2,2,2,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,3,3,2,3,2,3,3,3,3,3,3,2,3,2,2,0,1,3,2,1,2,2,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,3,3,3,2,1,1,3,0,1,1,1,1,2,1,1,0,2,2,2,1,2,0,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,3,3,2,2,2,2,1,3,2,3,2,3,2,1,2,2,0,1,1,2,1,2,1,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,3,3,3,3,3,3,2,2,3,2,3,3,3,2,2,2,2,0,2,2,2,2,3,1,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,
3,2,3,2,2,3,3,3,3,3,3,3,3,3,1,3,2,0,0,3,3,3,3,2,3,3,3,3,2,3,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,3,3,3,3,3,2,2,3,3,0,2,1,0,3,2,3,2,3,0,0,1,2,0,0,1,0,1,2,1,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,0,3,0,2,3,3,3,3,2,3,3,3,3,1,2,2,0,0,2,3,2,2,2,3,2,3,2,2,3,0,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,2,3,0,2,3,2,3,0,1,2,3,3,2,0,2,3,0,0,2,3,2,2,0,1,3,1,3,2,2,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,1,3,0,2,3,3,3,3,3,3,3,3,2,1,3,2,0,0,2,2,3,3,3,2,3,3,0,2,2,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,2,3,3,2,2,2,3,3,0,0,1,1,1,1,1,2,0,0,1,1,1,1,0,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,2,3,3,3,3,3,3,3,0,3,2,3,3,2,3,2,0,2,1,0,1,1,0,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,3,2,3,3,3,2,2,2,2,3,1,3,2,3,1,1,2,1,0,2,2,2,2,1,3,1,0,
0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,
2,2,3,3,3,3,3,1,2,2,1,3,1,0,3,0,0,3,0,0,0,1,1,0,1,2,1,0,0,0,0,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,2,2,1,1,3,3,3,2,2,1,2,2,3,1,1,2,0,0,2,2,1,3,0,0,2,1,1,2,1,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,2,3,3,3,3,1,2,2,2,1,2,1,3,3,1,1,2,1,2,1,2,2,0,2,0,0,1,1,0,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,3,3,3,3,3,2,1,3,2,2,3,2,0,3,2,0,3,0,1,0,1,1,0,0,1,1,1,1,0,1,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,2,3,3,3,2,2,2,3,3,1,2,1,2,1,0,1,0,1,1,0,1,0,0,2,1,1,1,0,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
3,1,1,2,1,2,3,3,2,2,1,2,2,3,0,2,1,0,0,2,2,3,2,1,2,2,2,2,2,3,1,0,
0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
3,3,3,3,3,1,1,0,1,1,2,2,1,1,3,0,0,1,3,1,1,1,0,0,0,1,0,1,1,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,1,3,3,3,2,0,0,0,2,1,0,1,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,0,1,0,0,2,3,2,2,2,1,2,2,2,1,2,1,0,0,1,1,1,0,2,0,1,1,1,0,0,1,1,
1,0,0,0,0,0,1,2,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
2,3,3,3,3,0,0,0,0,1,0,0,0,0,3,0,1,2,1,0,0,0,0,0,0,0,1,1,0,0,1,1,
1,0,1,0,1,2,0,0,1,1,2,1,0,1,1,1,1,0,1,1,1,1,0,1,0,0,1,0,0,1,1,0,
2,2,3,2,2,2,3,1,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,0,1,0,1,1,1,0,2,1,
1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,0,1,1,1,0,1,1,0,
3,3,3,2,2,2,2,3,2,2,1,1,2,2,2,2,1,1,3,1,2,1,2,0,0,1,1,0,1,0,2,1,
1,1,1,1,1,2,1,0,1,1,1,1,0,1,0,0,1,1,0,0,1,0,1,0,0,1,0,0,0,1,1,0,
2,0,0,1,0,3,2,2,2,2,1,2,1,2,1,2,0,0,0,2,1,2,2,1,1,2,2,0,1,1,0,2,
1,1,1,1,1,0,1,1,1,2,1,1,1,2,1,0,1,2,1,1,1,1,0,1,1,1,0,0,1,0,0,1,
1,3,2,2,2,1,1,1,2,3,0,0,0,0,2,0,2,2,1,0,0,0,0,0,0,1,0,0,0,0,1,1,
1,0,1,1,0,1,0,1,1,0,1,1,0,2,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,1,1,0,
2,3,2,3,2,1,2,2,2,2,1,0,0,0,2,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,2,1,
1,1,2,1,0,2,0,0,1,0,1,0,0,1,0,0,1,1,0,1,1,0,0,0,0,0,1,0,0,0,0,0,
3,0,0,1,0,2,2,2,3,2,2,2,2,2,2,2,0,0,0,2,1,2,1,1,1,2,2,0,0,0,1,2,
1,1,1,1,1,0,1,2,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,
2,3,2,3,3,2,0,1,1,1,0,0,1,0,2,0,1,1,3,1,0,0,0,0,0,0,0,1,0,0,2,1,
1,1,1,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,0,1,1,0,1,0,0,0,0,0,0,1,0,
2,3,3,3,3,1,2,2,2,2,0,1,1,0,2,1,1,1,2,1,0,1,1,0,0,1,0,1,0,0,2,0,
0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
2,3,3,3,2,0,0,1,1,2,2,1,0,0,2,0,1,1,3,0,0,1,0,0,0,0,0,1,0,1,2,1,
1,1,2,0,1,1,1,0,1,0,1,1,0,1,0,1,1,1,1,0,1,0,0,0,0,0,0,1,0,1,1,0,
1,3,2,3,2,1,0,0,2,2,2,0,1,0,2,0,1,1,1,0,1,0,0,0,3,0,1,1,0,0,2,1,
1,1,1,0,1,1,0,0,0,0,1,1,0,1,0,0,2,1,1,0,1,0,0,0,1,0,1,0,0,1,1,0,
3,1,2,1,1,2,2,2,2,2,2,1,2,2,1,1,0,0,0,2,2,2,0,0,0,1,2,1,0,1,0,1,
2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,2,1,1,1,0,1,0,1,1,0,1,1,1,0,0,1,
3,0,0,0,0,2,0,1,1,1,1,1,1,1,0,1,0,0,0,1,1,1,0,1,0,1,1,0,0,1,0,1,
1,1,0,0,1,0,0,0,1,0,1,1,0,0,1,0,1,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,
1,3,3,2,2,0,0,0,2,2,0,0,0,1,2,0,1,1,2,0,0,0,0,0,0,0,0,1,0,0,2,1,
0,1,1,0,0,1,1,0,0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,
2,3,2,3,2,0,0,0,0,1,1,0,0,0,2,0,2,0,2,0,0,0,0,0,1,0,0,1,0,0,1,1,
1,1,2,0,1,2,1,0,1,1,2,1,1,1,1,1,2,1,1,0,1,0,0,1,1,1,1,1,0,1,1,0,
1,3,2,2,2,1,0,0,2,2,1,0,1,2,2,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
1,0,0,1,0,2,3,1,2,2,2,2,2,2,1,1,0,0,0,1,0,1,0,2,1,1,1,0,0,0,0,1,
1,1,0,1,1,0,1,1,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
2,0,2,0,0,1,0,3,2,1,2,1,2,2,0,1,0,0,0,2,1,0,0,2,1,1,1,1,0,2,0,2,
2,1,1,1,1,1,1,1,1,1,1,1,1,2,1,0,1,1,1,1,0,0,0,1,1,1,1,0,1,0,0,1,
1,2,2,2,2,1,0,0,1,0,0,0,0,0,2,0,1,1,1,1,0,0,0,0,1,0,1,2,0,0,2,0,
1,0,1,1,1,2,1,0,1,0,1,1,0,0,1,0,1,1,1,0,1,0,0,0,1,0,0,1,0,1,1,0,
2,1,2,2,2,0,3,0,1,1,0,0,0,0,2,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
0,0,0,1,1,1,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,
1,2,2,3,2,2,0,0,1,1,2,0,1,2,1,0,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,1,
0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,1,1,0,
2,2,1,1,2,1,2,2,2,2,2,1,2,2,0,1,0,0,0,1,2,2,2,1,2,1,1,1,1,1,2,1,
1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,1,1,1,0,0,0,0,1,1,1,0,1,1,0,0,1,
1,2,2,2,2,0,1,0,2,2,0,0,0,0,2,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,2,0,
0,0,1,0,0,1,0,0,0,0,1,0,1,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,2,2,2,2,0,0,0,2,2,2,0,1,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
0,1,1,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,2,2,2,2,0,0,0,0,1,0,0,1,1,2,0,0,0,0,1,0,1,0,0,1,0,0,2,0,0,0,1,
0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,
1,2,2,2,1,1,2,0,2,1,1,1,1,0,2,2,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,1,
0,0,1,0,1,1,0,0,0,0,1,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
1,0,2,1,2,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,
0,0,1,0,1,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,
1,0,0,0,0,2,0,1,2,1,0,1,1,1,0,1,0,0,0,1,0,1,0,0,1,0,1,0,0,0,0,1,
0,0,0,0,0,1,0,0,1,1,0,0,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,
2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,
2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
1,1,1,0,1,0,1,0,0,1,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,0,0,0,0,
1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
1,1,0,1,1,0,1,0,1,0,0,0,0,1,1,0,1,1,0,0,0,0,0,1,0,1,1,0,1,0,0,0,
0,1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
};


const SequenceModel Koi8rModel = 
{
  KOI8R_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "KOI8-R"
};

const SequenceModel Win1251Model = 
{
  win1251_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "windows-1251"
};

const SequenceModel Latin5Model = 
{
  latin5_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "ISO-8859-5"
};

const SequenceModel MacCyrillicModel = 
{
  macCyrillic_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "x-mac-cyrillic"
};

const SequenceModel Ibm866Model = 
{
  IBM866_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "IBM866"
};

const SequenceModel Ibm855Model = 
{
  IBM855_CharToOrderMap,
  RussianLangModel,
  (float)0.976601,
  PR_FALSE,
  "IBM855"
};
