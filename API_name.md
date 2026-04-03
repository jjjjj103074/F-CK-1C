# F-CK-1C Blender / DCS API 命名對照

這份文件整理目前專案裡「必須和 Blender 匯出名稱一致」的部分，方便回頭改模型、hitbox、材質或碰撞殼時對照。

重點原則：

1. Blender 物件名、Shell 物件名、材質名，匯出成 EDM 後通常都會被原樣保留。
2. DCS Lua 設定引用的是匯出後的名稱，所以大小寫、空白、底線都要完全一致。
3. 目前這個專案同時存在「基線命名」與「現行實際命名」兩套；實際生效的名稱要以目前 EDM 與 Lua 檔案為準。
4. 有些名稱本身有拼字問題，例如 IDF_Galsses，但因為模型和 livery 都已經用這個名字，現階段不能直接改掉，除非 Blender、EDM、description.lua 一起同步改。

## 1. 模型檔與外層註冊名稱

| 實際名稱 | 用途 | Blender 名稱 / 匯出名稱 |
| --- | --- | --- |
| F-CK-1C | 主模型名稱。由 F-CK-1C.lua 的 shape_table_data.file 指向，DCS 會去找 Shapes/F-CK-1C.edm。 | 匯出的主模型檔名必須是 F-CK-1C.edm |
| idf_hitbox | 碰撞殼模型名稱。由 Shapes/F-CK-1C.lods 的 collision_shell 指向 idf_hitbox.edm。 | 匯出的碰撞殼檔名必須是 idf_hitbox.edm |

## 2. 起落架碰撞殼 API 名稱

這一組名稱是目前真正被 FM/config.lua 使用的碰撞殼名稱。這些名字必須和 idf_hitbox.edm 裡的 Shell Node 名稱完全一致。

| 實際名稱 | 用途 | Blender 名稱 |
| --- | --- | --- |
| F W | 前輪碰撞殼。FM/config.lua nose gear 的 collision_shell_name 目前就是這個值。 | Blender 中前輪碰撞殼物件請命名為 F W |
| LBW | 左主輪碰撞殼。FM/config.lua left main gear 的 collision_shell_name 目前就是這個值。 | Blender 中左主輪碰撞殼物件請命名為 LBW |
| RBW | 右主輪碰撞殼。FM/config.lua right main gear 的 collision_shell_name 目前就是這個值。 | Blender 中右主輪碰撞殼物件請命名為 RBW |

補充：

- 基線模板原本使用的是 WHEEL_F、WHEEL_L、WHEEL_R。
- 本專案目前實際 EDM 裡不是這三個名字，而是 F W、LBW、RBW。
- 如果未來想改回 WHEEL_F/WHEEL_L/WHEEL_R，必須同時改 Blender 匯出名稱和 FM/config.lua。

## 3. 目前在 hitbox EDM 內看得到的碰撞殼 / Shell 節點

這些名字是從 Shapes/IDF_Hitbox_strings_regen.txt 觀察到的可讀名稱。不是每一個都已經被 Lua 直接引用，但它們是 Blender 端很可能需要維持的碰撞分塊名稱。

### 3.1 已確認為可用 Shell 名稱

| 實際名稱 | 用途 | Blender 名稱 |
| --- | --- | --- |
| body | 機身主碰撞體。Lua 目前未直接引用，但屬於 hitbox 主要殼體。 | Blender shell 物件名維持 body |
| Tail | 尾部碰撞體。Lua 目前未直接引用，但可作為尾部 shell 區塊。 | Blender shell 物件名維持 Tail |
| Blap | 左側襟翼或左側後緣控制面碰撞體。 | Blender shell 物件名維持 Blap |
| Brap | 右側襟翼或右側後緣控制面碰撞體。 | Blender shell 物件名維持 Brap |
| Flap | 一個襟翼碰撞體名稱。 | Blender shell 物件名維持 Flap |
| Flap.001 | 另一個襟翼碰撞體名稱。 | Blender shell 物件名維持 Flap.001 |

### 3.2 已觀察到的其他 hitbox 節點

這些名稱也存在於 hitbox EDM 中，但目前沒有對應的 Lua 直接引用，暫時把它們當作碰撞分塊或內部結構節點保留。

| 實際名稱 | 用途 | Blender 名稱 |
| --- | --- | --- |
| IDF_Lflap | 左襟翼相關 hitbox 節點 | Blender 物件名維持 IDF_Lflap |
| IDF_Rflap | 右襟翼相關 hitbox 節點 | Blender 物件名維持 IDF_Rflap |
| IDF_Tail | 尾部相關 hitbox 節點 | Blender 物件名維持 IDF_Tail |
| TS_L | 左尾翼或左尾段碰撞節點 | Blender 物件名維持 TS_L |
| TS_R | 右尾翼或右尾段碰撞節點 | Blender 物件名維持 TS_R |
| IDF_FSB | 前機身或前煞車殼相關節點 | Blender 物件名維持 IDF_FSB |
| IDF_LBSD01 | 左側煞車板或左側分塊節點 01 | Blender 物件名維持 IDF_LBSD01 |
| IDF_LBSD02 | 左側煞車板或左側分塊節點 02 | Blender 物件名維持 IDF_LBSD02 |
| IDF_LBSD03 | 左側煞車板或左側分塊節點 03 | Blender 物件名維持 IDF_LBSD03 |
| IDF_RBSD01 | 右側煞車板或右側分塊節點 01 | Blender 物件名維持 IDF_RBSD01 |
| IDF_RBSD02 | 右側煞車板或右側分塊節點 02 | Blender 物件名維持 IDF_RBSD02 |
| IDF_RBSD03 | 右側煞車板或右側分塊節點 03 | Blender 物件名維持 IDF_RBSD03 |

## 4. 主模型材質 / 貼圖槽名稱

這一組名稱是目前 livery description.lua 與主模型 EDM 共同使用的材質名。這些名稱要和 Blender 材質名稱一致，DCS 才能正確套用貼圖。

| 實際名稱 | 用途 | Blender 材質名稱 |
| --- | --- | --- |
| IDF_Lwing | 左翼主材質 | Blender 材質名必須是 IDF_Lwing |
| IDF_Rwing | 右翼主材質 | Blender 材質名必須是 IDF_Rwing |
| IDF_TAIL | 尾翼主材質。livery 現在引用的是全大寫版本。 | Blender 材質名必須是 IDF_TAIL |
| IDF_Tail | 主模型內另有這個大小寫不同的名稱。若 Blender 內也有這個材質，需分開看待，不能和 IDF_TAIL 混用。 | Blender 材質名若存在就必須是 IDF_Tail |
| IDF_GEARS | 起落架材質 | Blender 材質名必須是 IDF_GEARS |
| IDF_Afterburn | 發動機後燃器 / 噴口相關材質 | Blender 材質名必須是 IDF_Afterburn |
| IDF_Launch_bra | 發射軌或掛架支架材質 | Blender 材質名必須是 IDF_Launch_bra |
| IDF_Glass | 座艙玻璃材質之一 | Blender 材質名必須是 IDF_Glass |
| IDF_Galsses | 另一個玻璃材質名稱，拼字雖然看起來不正確，但目前是有效名稱。 | Blender 材質名必須是 IDF_Galsses |
| IDF_BLOODY01 | 損傷 / 血污 / 破損覆蓋材質 01 | Blender 材質名必須是 IDF_BLOODY01 |
| IDF_BLOODY02 | 損傷 / 血污 / 破損覆蓋材質 02 | Blender 材質名必須是 IDF_BLOODY02 |
| IDF_BLOODY03 | 損傷 / 血污 / 破損覆蓋材質 03 | Blender 材質名必須是 IDF_BLOODY03 |
| IDF_BLOODY04 | 損傷 / 血污 / 破損覆蓋材質 04 | Blender 材質名必須是 IDF_BLOODY04 |

## 5. 主模型內可辨識的機構 / 骨架節點名稱

這些名字來自主模型 EDM 的可讀節點。它們不一定是 Lua 直接用名字控制，很多動畫其實是靠 EDM arg 編號驅動；但如果 Blender 內部原本就靠這些物件或骨架分件，建議維持名稱一致，避免重匯出後結構跑掉。

| 實際名稱 | 用途 | Blender 名稱 |
| --- | --- | --- |
| IDF_LG | 左主起落架總成節點 | Blender 物件或骨架名維持 IDF_LG |
| IDF_RG | 右主起落架總成節點 | Blender 物件或骨架名維持 IDF_RG |
| IDF_FG | 前起落架總成節點 | Blender 物件或骨架名維持 IDF_FG |
| IDF_FG_turn | 前輪轉向節點 | Blender 物件或骨架名維持 IDF_FG_turn |
| IDF_L_weee | 左輪胎節點 | Blender 物件或骨架名維持 IDF_L_weee |
| IDF_R_weee | 右輪胎節點 | Blender 物件或骨架名維持 IDF_R_weee |
| IDF_F_weee | 前輪胎節點 | Blender 物件或骨架名維持 IDF_F_weee |
| IDF_HOOK | 尾鉤或相關節點 | Blender 物件或骨架名維持 IDF_HOOK |
| Launch_bar.001 ~ Launch_bar.006 | 發射桿 / 連桿節點群 | Blender 物件名保留原命名 |
| After_burnerL | 左發後燃器節點 | Blender 物件名維持 After_burnerL |
| After_burnerR | 右發後燃器節點 | Blender 物件名維持 After_burnerR |

補充：

- 在 EDM 字串裡看到的 xxxtransform，通常表示 Blender 物件 xxx 在匯出後形成對應的 transform node。
- 實際需要在 Blender 裡維持的是原始物件名，例如 body、LBW、IDF_FG，不是帶 transform 後綴的名字。

## 6. 目前 Lua 端真的有直接綁名字的地方

### 6.1 FM/config.lua

這裡目前只有起落架碰撞殼名稱直接綁定到 hitbox shell 名稱：

- F W
- LBW
- RBW

### 6.2 Livery description.lua

這裡目前直接綁定材質名稱：

- IDF_Lwing
- IDF_Rwing
- IDF_TAIL
- IDF_GEARS
- IDF_Afterburn
- IDF_BLOODY01
- IDF_BLOODY02
- IDF_BLOODY03
- IDF_BLOODY04
- IDF_Launch_bra
- IDF_Glass
- IDF_Galsses

### 6.3 Shapes/F-CK-1C.lods

這裡直接綁定碰撞殼檔名：

- idf_hitbox.edm

### 6.4 F-CK-1C.lua

這裡直接綁定主模型檔名：

- F-CK-1C.edm

## 7. Blender 命名實務建議

1. 材質名不要只改大小寫，因為 DCS 對大小寫和拼字是實名匹配。
2. 碰撞殼若已經在 Lua 用到，就不要在 Blender 任意更名，尤其是 F W、LBW、RBW 這三個。
3. 如果要把歷史命名整理乾淨，例如把 F W 改成 WHEEL_F，必須同步改三個地方：Blender 物件名、匯出的 EDM、Lua 設定。
4. 拼字看起來錯的名稱也不要直接修，例如 IDF_Galsses，除非你同時改模型材質、貼圖檔名與所有 livery description.lua。
5. 先以「目前已經被 Lua 或 livery 直接引用的名稱」為最高優先保留對象。

## 8. 待後續補完的區域

以下部分目前還沒有完整 API 對照，之後若 Blender 原檔補齊，可以再擴充：

- 武器掛點 connector 名稱
- 燈光 connector 名稱
- 艙罩、起落架艙門、控制面對應的 EDM arg 編號對照
- damage cell 名稱與 Blender 碰撞分塊的一一對應

目前這份文件先以「已在專案中驗證存在的名稱」為準。
