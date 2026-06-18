$repo = Split-Path -Parent $MyInvocation.MyCommand.Path
$shapes = Join-Path $repo 'Shapes'
$trash = Join-Path $shapes 'trash'
if (-not (Test-Path $trash)) { New-Item -Path $trash -ItemType Directory | Out-Null }
$files = @(
'F-CK-1C.edm.bak',
'F-CK-1C.edm.bak.old',
'F-CK-1C.invalid_format_backup.edm',
'F-CK-1C.lods.idfts1_ground_test.old',
'F-CK-1C.lods.old',
'F-CK-1C.lods.pre_damage_investigation.old',
'F-CK-1C.lods.su30_fullswap_test.old',
'F-CK-1C.lods.su30_invalid_test.old',
'F-CK-1C_baseline_strings.txt',
'F-CK-1C_current_strings.txt',
'F-CK-1C_strings_regen.txt',
'IDFTS1.FCK_GROUND_TEST.edm',
'idf_hitbox.edm.bak',
'idf_hitbox.edm.old',
'IDF_Hitbox_strings.txt',
'su-30mk-collision.FCK_TEST.edm',
'su-30mk-collision_TEST.edm',
'Su-30MK.FCK_TEST.edm'
)

foreach ($f in $files) {
    $src = Join-Path $shapes $f
    if (Test-Path $src) {
        Move-Item -Path $src -Destination $trash -Force
        Write-Output "Moved: $f"
    } else {
        Write-Output "Missing: $f"
    }
}

Get-ChildItem -Path $trash -Name
