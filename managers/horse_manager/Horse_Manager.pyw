import os
import re
import sys
import glob
import shutil
import subprocess
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter import colorchooser
from tkinter import font as tkfont

APP_NAME = "RDR2 Horse Manager"
APP_VERSION = "2.0.0"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
VANILLA_DIR = os.path.join(BASE_DIR, "Vanilla")
MODDED_DIR = os.path.join(BASE_DIR, "Modded\lml\stream")


def joaat(text):
    h = 0
    for ch in text.lower().encode("utf-8"):
        h = (h + ch) & 0xFFFFFFFF
        h = (h + (h << 10)) & 0xFFFFFFFF
        h ^= (h >> 6)
    h = (h + (h << 3)) & 0xFFFFFFFF
    h ^= (h >> 11)
    h = (h + (h << 15)) & 0xFFFFFFFF
    return h



class Tag(object):
    OUTFITS = "UNK_MEMBER_0x124563FF"
    EXPLICIT_ASSETS = "UNK_MEMBER_0x685A958D"
    DRAWABLE = "UNK_MEMBER_0xC720CAAA"
    ALBEDO = "UNK_MEMBER_0xC8D27281"
    NORMAL = "normal"
    MATERIAL = "UNK_MEMBER_0x7A84E7BF"
    PALETTE = "UNK_MEMBER_0x66771820"
    TINT0 = "UNK_MEMBER_0x48EA2F7E"
    TINT1 = "UNK_MEMBER_0x368C8AC3"
    TINT2 = "UNK_MEMBER_0xB69E0AE8"
    PROBABILITY = "probability"
    SCALE = "scale"
    SCALE_RANDOM_OFFSET = "UNK_MEMBER_0x0FA9BAB7"
    EYE_REDNESS = "UNK_MEMBER_0x6DE241BD"
    EXPRESSIONS = "UNK_MEMBER_0x86CF602A"


    PART_BODY = "Body / Coat"
    PART_HEAD = "Head"
    PART_EYES = "Eyes"
    PART_EYELASHES = "Eyelashes"
    PART_TEARLINE = "Tearline"
    PART_EYEBALL = "Eyeball"
    PART_MANE = "Mane"
    PART_TAIL = "Tail"
    PART_HOOVES = "Hooves"
    PART_FEATHERS = "Feathers"
    PART_MUZZLE = "Muzzle"
    PART_OTHER = "Other"

    PART_ORDER = (PART_BODY, PART_HEAD, PART_EYES, PART_EYELASHES,
                  PART_TEARLINE, PART_EYEBALL, PART_MANE, PART_TAIL,
                  PART_HOOVES, PART_FEATHERS, PART_MUZZLE, PART_OTHER)


HORSE_NAMES = {
    "a_c_horse_americanpaint_greyovero": "American Paint - Gray Overo Coat",
    "a_c_horse_americanpaint_overo": "American Paint - Overo Coat",
    "a_c_horse_americanpaint_splashedwhite": "American Paint - Splashed White Coat",
    "a_c_horse_americanpaint_tobiano": "American Paint - Tobiano Coat",
    "a_c_horse_americanstandardbred_black": "American Standardbred - Black Coat",
    "a_c_horse_americanstandardbred_buckskin": "American Standardbred - Buckskin Coat",
    "a_c_horse_americanstandardbred_lightbuckskin": "American Standardbred - Light Buckskin Coat",
    "a_c_horse_americanstandardbred_palominodapple": "American Standardbred - Palomino Dapple Coat",
    "a_c_horse_americanstandardbred_silvertailbuckskin": "American Standardbred - Silver Tail Buckskin Coat",
    "a_c_horse_andalusian_darkbay": "Andalusian - Dark Bay Coat",
    "a_c_horse_andalusian_perlino": "Andalusian - Perlino Coat",
    "a_c_horse_andalusian_rosegray": "Andalusian - Rose Gray Coat",
    "a_c_horse_appaloosa_blacksnowflake": "Appaloosa - Black Snowflake",
    "a_c_horse_appaloosa_blanket": "Appaloosa - Blanket Coat",
    "a_c_horse_appaloosa_brownleopard": "Appaloosa - Brown Leopard Coat",
    "a_c_horse_appaloosa_fewspotted_pc": "Appaloosa - Few Spotted Coat",
    "a_c_horse_appaloosa_leopard": "Appaloosa - Leopard Coat",
    "a_c_horse_appaloosa_leopardblanket": "Appaloosa - Leopard Blanket Coat",
    "a_c_horse_arabian_black": "Arabian - Black Coat",
    "a_c_horse_arabian_grey": "Arabian - Gray Coat",
    "a_c_horse_arabian_redchestnut": "Arabian - Red Chestnut Coat",
    "a_c_horse_arabian_redchestnut_pc": "Arabian - Red Chestnut Coat",
    "a_c_horse_arabian_rosegreybay": "Arabian - Rose Gray Bay Coat",
    "a_c_horse_arabian_warpedbrindle_pc": "Arabian - Warped Brindle Coat",
    "a_c_horse_arabian_white": "Arabian - White Coat",
    "a_c_horse_ardennes_bayroan": "Ardennes - Bay Roan Coat",
    "a_c_horse_ardennes_irongreyroan": "Ardennes - Iron Gray Roan Coat",
    "a_c_horse_ardennes_strawberryroan": "Ardennes - Strawberry Roan Coat",
    "a_c_horse_belgian_blondchestnut": "Belgian Draft - Blond Chestnut Coat",
    "a_c_horse_belgian_mealychestnut": "Belgian Draft - Mealy Chestnut Coat",
    "a_c_horse_breton_grullodun": "Breton - Grullo Dun Coat",
    "a_c_horse_breton_mealydapplebay": "Breton - Mealy Dapple Bay Coat",
    "a_c_horse_breton_redroan": "Breton - Red Roan Coat",
    "a_c_horse_breton_sealbrown": "Breton - Seal Brown Coat",
    "a_c_horse_breton_sorrel": "Breton - Sorrel Coat",
    "a_c_horse_breton_steelgrey": "Breton - Steel Gray Coat",
    "a_c_horse_buell_warvets": "Dutch Warmblood - Cremello Gold Coat",
    "a_c_horse_criollo_baybrindle": "Criollo - Bay Brindle Coat",
    "a_c_horse_criollo_bayframeovero": "Criollo - Bay Frame Overo Coat",
    "a_c_horse_criollo_blueroanovero": "Criollo - Blue Roan Overo Coat",
    "a_c_horse_criollo_dun": "Criollo - Dun Coat",
    "a_c_horse_criollo_marblesabino": "Criollo - Marble Sabino Coat",
    "a_c_horse_criollo_sorrelovero": "Criollo - Sorrel Overo Coat",
    "a_c_horse_dutchwarmblood_chocolateroan": "Dutch Warmblood - Chocolate Roan Coat",
    "a_c_horse_dutchwarmblood_sealbrown": "Dutch Warmblood - Seal Brown Coat",
    "a_c_horse_dutchwarmblood_sootybuckskin": "Dutch Warmblood - Sooty Buckskin Coat",
    "a_c_horse_eagleflies": "American Paint - Splashed White Coat",
    "a_c_horse_gang_bill": "Ardennes - Chestnut Coat",
    "a_c_horse_gang_charles": "Appaloosa - Gray Snowcap Spotted Coat",
    "a_c_horse_gang_charles_endlesssummer": "Nokota - Gray Roan Sabino Coat",
    "a_c_horse_gang_dutch": "Arabian - White Albino Coat",
    "a_c_horse_gang_hosea": "Turkoman - Silver Coat",
    "a_c_horse_gang_javier": "American Paint - Gray Overo Coat",
    "a_c_horse_gang_john": "Hungarian Halfbred - Silver Dark Bay Coat",
    "a_c_horse_gang_karen": "Nokota - Smoky Black Coat",
    "a_c_horse_gang_kieran": "Tennessee Walker - Flaxen Roan",
    "a_c_horse_gang_lenny": "Mustang - Light Palomino Coat",
    "a_c_horse_gang_micah": "Missouri Fox Trotter - Black Coat",
    "a_c_horse_gang_sadie": "Turkoman - Gold Dapple Coat",
    "a_c_horse_gang_sadie_endlesssummer": "Mustang - Dark Bay Roan Coat",
    "a_c_horse_gang_sean": "American Standardbred - Silver Tail Buckskin Coat",
    "a_c_horse_gang_trelawney": "Appaloosa - Brown Leopard Coat",
    "a_c_horse_gang_uncle": "Kentucky Saddler - Sabino Coat",
    "a_c_horse_gang_uncle_endlesssummer": "Appaloosa - Few Spot Buckskin Coat",
    "a_c_horse_gypsycob_palominoblagdon": "Gypsy Cob - Palomino Blagdon Coat",
    "a_c_horse_gypsycob_piebald": "Gypsy Cob - Piebald Coat",
    "a_c_horse_gypsycob_skewbald": "Gypsy Cob - Skewbald Coat",
    "a_c_horse_gypsycob_splashedbay": "Gypsy Cob - Splashed Bay Coat",
    "a_c_horse_gypsycob_splashedpiebald": "Gypsy Cob - Splashed Piebald Coat",
    "a_c_horse_gypsycob_whiteblagdon": "Gypsy Cob - White Blagdon Coat",
    "a_c_horse_hungarianhalfbred_darkdapplegrey": "Hungarian Halfbred - Dark Dapple Gray Coat",
    "a_c_horse_hungarianhalfbred_flaxenchestnut": "Hungarian Halfbred - Flaxen Chestnut Coat",
    "a_c_horse_hungarianhalfbred_liverchestnut": "Hungarian Halfbred - Liver Chestnut Coat",
    "a_c_horse_hungarianhalfbred_piebaldtobiano": "Hungarian Halfbred - Piebald Tobiano Coat",
    "a_c_horse_john_endlesssummer": "Thoroughbred - Seal Brown Coat",
    "a_c_horse_kentuckysaddle_black": "Kentucky Saddler - Black Coat",
    "a_c_horse_kentuckysaddle_buttermilkbuckskin_pc": "Kentucky Saddler - Buttermilk Buckskin Coat",
    "a_c_horse_kentuckysaddle_chestnutpinto": "Kentucky Saddler - Chestnut Pinto Coat",
    "a_c_horse_kentuckysaddle_grey": "Kentucky Saddler - Gray Coat",
    "a_c_horse_kentuckysaddle_silverbay": "Kentucky Saddler - Silver Bay Coat",
    "a_c_horse_kladruber_black": "Kladruber - Black Coat",
    "a_c_horse_kladruber_cremello": "Kladruber - Cremello Coat",
    "a_c_horse_kladruber_dapplerosegrey": "Kladruber - Dapple Rose Gray Coat",
    "a_c_horse_kladruber_grey": "Kladruber - Gray Coat",
    "a_c_horse_kladruber_silver": "Kladruber - Silver Coat",
    "a_c_horse_kladruber_white": "Kladruber - White Coat",
    "a_c_horse_missourifoxtrotter_amberchampagne": "Missouri Fox Trotter - Amber Champagne Coat",
    "a_c_horse_missourifoxtrotter_blacktovero": "Missouri Fox Trotter - Black Tovero Coat",
    "a_c_horse_missourifoxtrotter_blueroan": "Missouri Fox Trotter - Blue Roan Coat",
    "a_c_horse_missourifoxtrotter_buckskinbrindle": "Missouri Fox Trotter - Buckskin Brindle Coat",
    "a_c_horse_missourifoxtrotter_dapplegrey": "Missouri Fox Trotter - Dapple Gray Coat",
    "a_c_horse_missourifoxtrotter_sablechampagne": "Missouri Fox Trotter - Sable Champagne Coat",
    "a_c_horse_missourifoxtrotter_silverdapplepinto": "Missouri Fox Trotter - Silver Dapple Pinto Coat",
    "a_c_horse_morgan_bay": "Morgan - Bay Coat",
    "a_c_horse_morgan_bayroan": "Morgan - Bay Roan Coat",
    "a_c_horse_morgan_flaxenchestnut": "Morgan - Flaxen Chestnut Coat",
    "a_c_horse_morgan_liverchestnut_pc": "Morgan - Liver Chestnut Coat",
    "a_c_horse_morgan_palomino": "Morgan - Palomino Coat",
    "a_c_horse_mp_mangy_backup": "Scrawny Nag",
    "a_c_horse_murfreebrood_mange_01": "Appaloosa - Blanket Coat",
    "a_c_horse_murfreebrood_mange_02": "Nokota - Blue Roan Coat",
    "a_c_horse_murfreebrood_mange_03": "Tennessee Walker - Black Rabicano Coat",
    "a_c_horse_mustang_blackovero": "Mustang - Black Overo Coat",
    "a_c_horse_mustang_buckskin": "Mustang - Buckskin Coat",
    "a_c_horse_mustang_chestnuttovero": "Mustang - Chestnut Tovero Coat",
    "a_c_horse_mustang_goldendun": "Mustang - Golden Dun Coat",
    "a_c_horse_mustang_grullodun": "Mustang - Grullo Dun Coat",
    "a_c_horse_mustang_reddunovero": "Mustang - Red Dun Overo Coat",
    "a_c_horse_mustang_tigerstripedbay": "Mustang - Tiger Striped Bay Coat",
    "a_c_horse_mustang_wildbay": "Mustang - Wild Bay Coat",
    "a_c_horse_nokota_blueroan": "Nokota - Blue Roan Coat",
    "a_c_horse_nokota_reversedappleroan": "Nokota - Reverse Dapple Roan Coat",
    "a_c_horse_nokota_whiteroan": "Nokota - White Roan Coat",
    "a_c_horse_norfolkroadster_black": "Norfolk Roadster - Black Coat",
    "a_c_horse_norfolkroadster_dappledbuckskin": "Norfolk Roadster - Dappled Buckskin Coat",
    "a_c_horse_norfolkroadster_piebaldroan": "Norfolk Roadster - Piebald Roan Coat",
    "a_c_horse_norfolkroadster_rosegrey": "Norfolk Roadster - Rose Gray Coat",
    "a_c_horse_norfolkroadster_speckledgrey": "Norfolk Roadster - Speckled Gray Coat",
    "a_c_horse_norfolkroadster_spottedtricolor": "Norfolk Roadster - Spotted Tricolor Coat",
    "a_c_horse_shire_darkbay": "Shire - Dark Bay Coat",
    "a_c_horse_shire_lightgrey": "Shire - Light Gray Coat",
    "a_c_horse_shire_ravenblack": "Shire - Raven Black Coat",
    "a_c_horse_suffolkpunch_redchestnut": "Suffolk Punch - Red Chestnut Coat",
    "a_c_horse_suffolkpunch_sorrel": "Suffolk Punch - Sorrel Coat",
    "a_c_horse_tennesseewalker_blackrabicano": "Tennessee Walker - Black Rabicano Coat",
    "a_c_horse_tennesseewalker_chestnut": "Tennessee Walker - Chestnut Coat",
    "a_c_horse_tennesseewalker_dapplebay": "Tennessee Walker - Dapple Bay Coat",
    "a_c_horse_tennesseewalker_flaxenroan": "Tennessee Walker - Flaxen Roan Coat",
    "a_c_horse_tennesseewalker_goldpalomino_pc": "Tennessee Walker - Gold Palomino Coat",
    "a_c_horse_tennesseewalker_mahoganybay": "Tennessee Walker - Mahogany Bay Coat",
    "a_c_horse_tennesseewalker_redroan": "Tennessee Walker - Red Roan Coat",
    "a_c_horse_thoroughbred_blackchestnut": "Thoroughbred - Black Chestnut Coat",
    "a_c_horse_thoroughbred_bloodbay": "Thoroughbred - Blood Bay Coat",
    "a_c_horse_thoroughbred_brindle": "Thoroughbred - Brindle Coat",
    "a_c_horse_thoroughbred_dapplegrey": "Thoroughbred - Dapple Gray Coat",
    "a_c_horse_thoroughbred_reversedappleblack": "Thoroughbred - Reverse Dapple Black Coat",
    "a_c_horse_turkoman_black": "Turkoman - Black Coat",
    "a_c_horse_turkoman_chestnut": "Turkoman - Chestnut Coat",
    "a_c_horse_turkoman_darkbay": "Turkoman - Dark Bay Coat",
    "a_c_horse_turkoman_gold": "Turkoman - Gold Coat",
    "a_c_horse_turkoman_grey": "Turkoman - Gray Coat",
    "a_c_horse_turkoman_perlino": "Turkoman - Perlino Coat",
    "a_c_horse_turkoman_silver": "Turkoman - Silver Coat",
    "a_c_horse_winter02_01": "Thoroughbred - Silver Bay Coat",
    "a_c_horsemule_01": "Mule",
    "a_c_horsemulepainted_01": "Mule - Painted",
}

BREED_NAMES = {
    "01": "Mule",
    "americanpaint": "American Paint",
    "americanstandardbred": "American Standardbred",
    "andalusian": "Andalusian",
    "appaloosa": "Appaloosa",
    "arabian": "Arabian",
    "ardennes": "Ardennes",
    "belgian": "Belgian Draft",
    "breton": "Breton",
    "buell": "Dutch Warmblood",
    "criollo": "Criollo",
    "dutchwarmblood": "Dutch Warmblood",
    "eagleflies": "American Paint",
    "gang": "Appaloosa",
    "gypsycob": "Gypsy Cob",
    "hungarianhalfbred": "Hungarian Halfbred",
    "john": "Thoroughbred",
    "kentuckysaddle": "Kentucky Saddler",
    "kladruber": "Kladruber",
    "missourifoxtrotter": "Missouri Fox Trotter",
    "morgan": "Morgan",
    "mp": "Scrawny Nag",
    "murfreebrood": "Tennessee Walker",
    "mustang": "Mustang",
    "nokota": "Nokota",
    "norfolkroadster": "Norfolk Roadster",
    "painted": "Mule",
    "shire": "Shire",
    "suffolkpunch": "Suffolk Punch",
    "tennesseewalker": "Tennessee Walker",
    "thoroughbred": "Thoroughbred",
    "turkoman": "Turkoman",
    "winter02": "Thoroughbred",
}

DRAWABLE_INFO = {
    0x019B9335: (Tag.PART_MANE, "mp_horse_01_mane_013"),
    0x05C1B233: (Tag.PART_FEATHERS, "mp_horse_01_feather_000"),
    0x062AA1DE: (Tag.PART_TAIL, "mp_horse_01_hair_000"),
    0x1E1A542F: (Tag.PART_EYELASHES, "p_c_horse_01_mis2_000"),
    0x287A3C54: (Tag.PART_MANE, "p_c_horse_01_mane_000"),
    0x2C372A0F: (Tag.PART_HEAD, "p_c_horse_01_head_001"),
    0x3378CD6E: (Tag.PART_TAIL, "p_c_horse_01_hair_002"),
    0x3813569F: (Tag.PART_TAIL, "p_c_horse_01_hair_000"),
    0x3DD0E701: (Tag.PART_MANE, "p_c_horse_01_mane_002"),
    0x3E83CEA8: (Tag.PART_HEAD, "p_c_horse_01_head_000"),
    0x408B84F2: (Tag.PART_FEATHERS, "p_c_horse_01_feather_000"),
    0x4253E81C: (Tag.PART_EYEBALL, "p_c_horse_01_eyes_000"),
    0x434D6D13: (Tag.PART_TAIL, "p_c_horse_01_hair_001"),
    0x4A4A3A1A: (Tag.PART_BODY, "p_c_horse_01_hand_000"),
    0x5104AB9C: (Tag.PART_OTHER, "p_c_horse_01_slvl_001"),
    0x53A5483F: (Tag.PART_BODY, "mp_horse_01_hand_000"),
    0x54198088: (Tag.PART_TEARLINE, "p_c_horse_01_tas2_000"),
    0x577FC48B: (Tag.PART_TAIL, "mp_horse_01_hair_001"),
    0x5D586036: (Tag.PART_BODY, "p_c_horse_01_hand_001"),
    0x61E12F21: (Tag.PART_MANE, "p_c_horse_01_mane_003"),
    0x756ED63C: (Tag.PART_MANE, "p_c_horse_01_mane_005"),
    0x7F34D68F: (Tag.PART_OTHER, "p_c_horse_01_acc3_001"),
    0x83989A52: (Tag.PART_OTHER, "p_c_horse_01_accs_001"),
    0x850C7577: (Tag.PART_MANE, "p_c_horse_01_mane_006"),
    0x8591E73A: (Tag.PART_HEAD, "mp_horse_01_head_000"),
    0xA4944E58: (Tag.PART_MANE, "mp_horse_01_mane_000"),
    0xA90EBD77: (Tag.PART_MANE, "p_c_horse_01_mane_007"),
    0xAF7D6111: (Tag.PART_OTHER, "p_c_horse_01_moustache_000"),
    0xB45BC8BF: (Tag.PART_EYES, "p_c_horse_01_teef_000"),
    0xBC8A646E: (Tag.PART_MANE, "p_c_horse_01_mane_009"),
    0xBED47173: (Tag.PART_OTHER, "p_c_horse_01_belt_001"),
    0xBF0FFB10: (Tag.PART_OTHER, "stirrup_hr1_008"),
    0xC5F58AEB: (Tag.PART_OTHER, "p_c_horse_01_acc2_001"),
    0xD140B63B: (Tag.PART_HOOVES, "p_c_horse_01_feet_001"),
    0xDC40C87C: (Tag.PART_MANE, "mp_horse_01_mane_011"),
    0xEC0D4E7D: (Tag.PART_OTHER, "saddlecombo_hr1_000"),
    0xEE3657D8: (Tag.PART_OTHER, "p_c_horse_01_uppr_002"),
    0xEE446C83: (Tag.PART_MANE, "mp_horse_01_mane_012"),
    0xF0FF8D7B: (Tag.PART_OTHER, "horn_hr1_003"),
    0xF2C450E1: (Tag.PART_MANE, "p_c_horse_01_mane_008"),
    0xF931D8E1: (Tag.PART_TAIL, "p_c_horse_01_hair_005"),
}

COAT_PAIRS = {
    0x021C830A: ("appy_blanket (appy)", 0x6A0C1F5F),
    0x02BEAB39: ("dwb_sealbrown", 0x07BD9AF8),
    0x041F78EF: ("norfolk spotted tricolor", 0x4074852B),
    0x047B2051: ("turk chestnut", 0xB2D7B85B),
    0x04A87DF0: ("mft buckskin brindle", 0x67E37518),
    0x05118693: ("norfolk rose grey", 0xF2FB01FE),
    0x06D1BFB5: ("turk_perlino (turk darkbay AND perlino, gang_dutch)", 0x1597BB51),
    0x0769D455: ("kentucky_pinto (kentuckly saddler)", 0xE9925F46),
    0x07BD9AF8: ("dwb_sealbrown", 0x02BEAB39),
    0x087A45CE: ("asb_silverbuckskin (asb silverbuckskin, arabian, shire, gang_sean)", 0xBB8EBE02),
    0x08A9FB5C: ("nokota_whiteroan", 0x25988B48),
    0x0961B7F3: ("gypsy palomino blagdon", 0x34E9D503),
    0x0AEEF8C5: ("mealy dapple bay (breton)", 0x2BC3F695),
    0x10180EAF: ("breton grullo dun", 0xEC6FF979),
    0x110C2A34: ("paint_overo (american paint)", 0x3E88A345),
    0x110F0797: ("belgian_mealy (belgian x2, shire, suffolk)", 0xEBFD74C8),
    0x11B758CA: ("criollo_dun (criollo, andalusian, morgan, mustang, tnw )", 0x3D20794F),
    0x13D64F54: ("charles_endless_summer", 0x285EDED4),
    0x1597BB51: ("turk_perlino (turk darkbay AND perlino, gang_dutch)", 0x06D1BFB5),
    0x16DB3FD7: ("breton red roan", 0x1E250DC1),
    0x1927D699: ("mustang red dun overo", 0x368EE8A6),
    0x1B46FE0C: ("mft blue roan", 0x3510E2FD),
    0x1B71CB2B: ("dwb_sooty_buckskin (dwb)", 0xCCDD64CE),
    0x1D03D28D: ("breton steel grey", 0x54B161BA),
    0x1E250DC1: ("breton red roan", 0x16DB3FD7),
    0x1EB04A80: ("uncle_endless_summer", 0x8FEAAEE4),
    0x231E0170: ("tb_brindle (tb)", 0xA3A799BD),
    0x238B4185: ("gypsy white blagdon", 0x484A6E9F),
    0x25988B48: ("nokota_whiteroan", 0x08A9FB5C),
    0x25ECEC41: ("gang_bill", 0xD59050DF),
    0x271370A2: ("gypsy piebald", 0x771E1CD1),
    0x27E50F8A: ("norfolk black", 0x62056C46),
    0x27E986CB: ("paint_grey_overo (american pint, gang_javier)", 0x2B3497F1),
    0x285EDED4: ("charles_endless_summer", 0x13D64F54),
    0x2B3497F1: ("paint_grey_overo (american pint, gang_javier)", 0x27E986CB),
    0x2B5BCC01: ("mft_dapplepinto", 0xDF3B429C),
    0x2B5C884B: ("turk_gold (asb, arabian, buell, hungarian, kentucky, morgan, shire, turk)", 0xA1AC42C9),
    0x2B668DB6: ("norfolk piebald roan", 0x8E84C5DF),
    0x2BC3F695: ("mealy dapple bay (breton)", 0x0AEEF8C5),
    0x2BFA9501: ("leopard_blanket (appy)", 0xFFFAE34A),
    0x2C25780A: ("norfolk speckled grey", 0x37CA5A02),
    0x2E44D3CB: ("criollo bay brindle", 0x5A60B202),
    0x34E9D503: ("gypsy palomino blagdon", 0x0961B7F3),
    0x3510E2FD: ("mft blue roan", 0x1B46FE0C),
    0x368EE8A6: ("mustang red dun overo", 0x1927D699),
    0x37CA5A02: ("norfolk speckled grey", 0x2C25780A),
    0x3C1AC078: ("simple blaze??? (arabian, hungarian, tnw, tb)", 0x490CD77A),
    0x3C3834FD: ("nokota_whiteroan", 0x686891C5),
    0x3D20794F: ("criollo_dun (criollo, andalusian, morgan, mustang, tnw )", 0x11B758CA),
    0x3DC33635: ("mft black tovero", 0xD5F5C9AF),
    0x3E88A345: ("paint_overo (american paint)", 0x110C2A34),
    0x4074852B: ("norfolk spotted tricolor", 0x041F78EF),
    0x41C38EEF: ("ardennes_strawberry_roan (suffolk, ardennes)", 0x9BB5E26B),
    0x42589694: ("morgan_liver_chestnut (morgan, gang_sadie)", 0x91CA4FB2),
    0x42D81168: ("tiger_striped_bay (mustang golden dun AND tiger striped bay)", 0x61F86FD2),
    0x4397733D: ("klad black and white", 0x63F49F9E),
    0x484A6E9F: ("gypsy white blagdon", 0x238B4185),
    0x48C1ABAB: ("gypsy splashed pibald", 0xFB721189),
    0x490CD77A: ("simple blaze??? (arabian, hungarian, tnw, tb)", 0x3C1AC078),
    0x491779BF: ("zebra", 0xA4D9B97A),
    0x49F39E30: ("norfolk dappled buckskin", 0x65C84192),
    0x4B280731: ("tnw_flaxenroan (tnw x2, gang_kieran)", 0x95D39DD2),
    0x4D96CB3A: ("gang_lenny (kentucky saddler, gang_lenny)", 0x9ED04A50),
    0x4DC849EB: ("breton seal brown", 0xFF030F50),
    0x4F85D574: ("criollo marble sabino", 0x78471A92),
    0x54B161BA: ("breton steel grey", 0x1D03D28D),
    0x56F05A18: ("turk grey", 0x834701C7),
    0x586F5DD0: ("appy_leopard (appy x2)", 0x64AFA858),
    0x5A60B202: ("criollo bay brindle", 0x2E44D3CB),
    0x5ADA1481: ("dark_dapple (andalusian/hungarian)", 0x654E2BA0),
    0x5B3F6EEE: ("baldface (tnw, gang_micah)", 0x9B092F21),
    0x5CFE74AF: ("gypsy skewbald", 0xF7196854),
    0x5E2EC866: ("mustang_wildbay (mustang, tb, gang_john)", 0x65BC843D),
    0x61F86FD2: ("tiger_striped_bay (mustang golden dun AND tiger striped bay)", 0x42D81168),
    0x62056C46: ("norfolk black", 0x27E50F8A),
    0x627031A6: ("dark_dapple (gang_uncle)", 0xF84E8ECC),
    0x63F49F9E: ("klad black and white", 0x4397733D),
    0x64AFA858: ("appy_leopard (appy x2)", 0x586F5DD0),
    0x654E2BA0: ("dark_dapple (andalusian/hungarian)", 0x5ADA1481),
    0x65A157C3: ("warped brindle (arabian)", 0x75D97405),
    0x65BC843D: ("mustang_wildbay (mustang, tb, gang_john)", 0x5E2EC866),
    0x65C84192: ("norfolk dappled buckskin", 0x49F39E30),
    0x67E37518: ("mft buckskin brindle", 0x04A87DF0),
    0x686891C5: ("nokota_whiteroan", 0x3C3834FD),
    0x691A3E50: ("criollo blue roan overo", 0x8C5358A2),
    0x6A0C1F5F: ("appy_blanket (appy)", 0x021C830A),
    0x6A89C927: ("mustang chestnut overo", 0xCE6266D1),
    0x6FEC5EB3: ("brown_leopard (appy)", 0xE7D5E9DD),
    0x717A8CCC: ("red_roan (tnw only)", 0xE98F788A),
    0x747E88A0: ("mustang buckskin", 0xF2958374),
    0x748FF64F: ("mft_champagne (arabian, mft x2)", 0xD24DD595),
    0x758D2297: ("klad cremello and silver", 0x9DCEF3B1),
    0x75D97405: ("warped brindle (arabian)", 0x65A157C3),
    0x76A9266B: ("mft dapple grey", 0x95EF1E85),
    0x771E1CD1: ("gypsy piebald", 0x271370A2),
    0x78471A92: ("criollo marble sabino", 0x4F85D574),
    0x795DE09C: ("hungarian_piebald_tobiano", 0xB5D143FA),
    0x7ABD97C8: ("criollo sorrel overo", 0xA6A4E113),
    0x7C06683F: ("tb_reverseddapple", 0xED2E8A67),
    0x7E7C8956: ("asb_palomino_dapple (asb, dutch warmblood, nokota, tb)", 0xC81C1AAC),
    0x834701C7: ("turk grey", 0x56F05A18),
    0x8BFF94C7: ("turk_silver (gang_hosea)", 0xF915D39F),
    0x8C5358A2: ("criollo blue roan overo", 0x691A3E50),
    0x8E84C5DF: ("norfolk piebald roan", 0x2B668DB6),
    0x8FEAAEE4: ("uncle_endless_summer", 0x1EB04A80),
    0x91CA4FB2: ("morgan_liver_chestnut (morgan, gang_sadie)", 0x42589694),
    0x95D39DD2: ("tnw_flaxenroan (tnw x2, gang_kieran)", 0x4B280731),
    0x95EF1E85: ("mft dapple grey", 0x76A9266B),
    0x9A6178C1: ("paint_tobiano (american paint, gang_karen)", 0xC43F7F29),
    0x9B092F21: ("baldface (tnw, gang_micah)", 0x5B3F6EEE),
    0x9BB5E26B: ("ardennes_strawberry_roan (suffolk, ardennes)", 0x41C38EEF),
    0x9DCEF3B1: ("klad cremello and silver", 0x758D2297),
    0x9ED04A50: ("gang_lenny (kentucky saddler, gang_lenny)", 0x4D96CB3A),
    0xA0992057: ("tnw_black_rabicano (tnw)", 0xBD1E8996),
    0xA1AC42C9: ("turk_gold (asb, arabian, buell, hungarian, kentucky, morgan, shire, turk)", 0x2B5C884B),
    0xA2FE8714: ("klad grey / dapple rose grey", 0xC0A1E263),
    0xA3A799BD: ("tb_brindle (tb)", 0x231E0170),
    0xA4D9B97A: ("zebra", 0x491779BF),
    0xA6A4E113: ("criollo sorrel overo", 0x7ABD97C8),
    0xA855E542: ("splashed_white (american paint, eagleflies)", 0xE658DD74),
    0xAA4C7C37: ("mustang black overo", 0xAF566330),
    0xAF566330: ("mustang black overo", 0xAA4C7C37),
    0xB2D7B85B: ("turk chestnut", 0x047B2051),
    0xB3252C13: ("gypsy splashed bay", 0xB87E3B6B),
    0xB3D4AF99: ("criollo bay frame overo", 0xDD1D4C53),
    0xB4FEED82: ("breton sorrel", 0xC7D856D2),
    0xB5D143FA: ("hungarian_piebald_tobiano", 0x795DE09C),
    0xB87E3B6B: ("gypsy splashed bay", 0xB3252C13),
    0xBB8EBE02: ("asb_silverbuckskin (asb silverbuckskin, arabian, shire, gang_sean)", 0x087A45CE),
    0xBD1E8996: ("tnw_black_rabicano (tnw)", 0xA0992057),
    0xC04AC71E: ("asb_bucskin (asb x2, andalusian, ardennes x2, kentucky x2, morgan, nokota)", 0xD14D40AE),
    0xC0A1E263: ("klad grey / dapple rose grey", 0xA2FE8714),
    0xC43F7F29: ("paint_tobiano (american paint, gang_karen)", 0x9A6178C1),
    0xC7D856D2: ("breton sorrel", 0xB4FEED82),
    0xC81C1AAC: ("asb_palomino_dapple (asb, dutch warmblood, nokota, tb)", 0x7E7C8956),
    0xCCDD64CE: ("dwb_sooty_buckskin (dwb)", 0x1B71CB2B),
    0xCE6266D1: ("mustang chestnut overo", 0x6A89C927),
    0xD14D40AE: ("asb_bucskin (asb x2, andalusian, ardennes x2, kentucky x2, morgan, nokota)", 0xC04AC71E),
    0xD24DD595: ("mft_champagne (arabian, mft x2)", 0x748FF64F),
    0xD59050DF: ("gang_bill", 0x25ECEC41),
    0xD5F5C9AF: ("mft black tovero", 0x3DC33635),
    0xD7DDE492: ("turk black", 0xE364BEC6),
    0xDD1D4C53: ("criollo bay frame overo", 0xB3D4AF99),
    0xDF3B429C: ("mft_dapplepinto", 0x2B5BCC01),
    0xE364BEC6: ("turk black", 0xD7DDE492),
    0xE658DD74: ("splashed_white (american paint, eagleflies)", 0xA855E542),
    0xE7D5E9DD: ("brown_leopard (appy)", 0x6FEC5EB3),
    0xE98F788A: ("red_roan (tnw only)", 0x717A8CCC),
    0xE9925F46: ("kentucky_pinto (kentuckly saddler)", 0x0769D455),
    0xEBFD74C8: ("belgian_mealy (belgian x2, shire, suffolk)", 0x110F0797),
    0xEC6FF979: ("breton grullo dun", 0x10180EAF),
    0xED2E8A67: ("tb_reverseddapple", 0x7C06683F),
    0xF2958374: ("mustang buckskin", 0x747E88A0),
    0xF2FB01FE: ("norfolk rose grey", 0x05118693),
    0xF7196854: ("gypsy skewbald", 0x5CFE74AF),
    0xF84E8ECC: ("dark_dapple (gang_uncle)", 0x627031A6),
    0xF915D39F: ("turk_silver (gang_hosea)", 0x8BFF94C7),
    0xFB721189: ("gypsy splashed pibald", 0x48C1ABAB),
    0xFF030F50: ("breton seal brown", 0x4DC849EB),
    0xFFFAE34A: ("leopard_blanket (appy)", 0x2BFA9501),
}

TEXTURE_NAMES = {
    0x021C830A: "p_c_horse_01_hand_000_c0_996_ab",
    0x02BEAB39: "p_c_horse_01_hand_000_c0_876_ab",
    0x041F78EF: "mp_horse_01_head_000_c0_839_ab",
    0x047B2051: "mp_horse_01_head_000_c0_829_ab",
    0x04A87DF0: "mp_horse_01_head_000_c0_822_ab",
    0x05118693: "mp_horse_01_head_000_c0_837_ab",
    0x066844BD: "saddlecombo_hr1_000_c4_000_ab",
    0x06D1BFB5: "p_c_horse_01_head_000_c0_872_ab",
    0x0769D455: "p_c_horse_01_head_000_c0_997_ab",
    0x07BD9AF8: "p_c_horse_01_head_000_c0_876_ab",
    0x087A45CE: "p_c_horse_01_hand_000_c0_881_ab",
    0x08A9FB5C: "p_c_horse_01_hand_000_c0_999_ab",
    0x0961B7F3: "mp_horse_01_hand_000_c0_842_ab",
    0x0B940125: "p_c_horse_01_hair_000_c0_999_ab",
    0x110C2A34: "p_c_horse_01_hand_000_c0_895_ab",
    0x110F0797: "p_c_horse_01_hand_000_c0_896_ab",
    0x11B758CA: "p_c_horse_01_head_000_c0_873_ab",
    0x13D64F54: "p_c_horse_01_hand_000_c0_864_ab",
    0x1597BB51: "p_c_horse_01_hand_000_c0_872_ab",
    0x1642DC23: "p_c_horse_01_acc3_001_c0_000_ab",
    0x185753CC: "p_c_horse_01_hand_000_c0_096_ab",
    0x1927D699: "mp_horse_01_head_000_c0_824_ab",
    0x1B46FE0C: "mp_horse_01_hand_000_c0_832_ab",
    0x1B71CB2B: "p_c_horse_01_head_000_c0_894_ab",
    0x1EB04A80: "p_c_horse_01_head_000_c0_865_ab",
    0x231E0170: "p_c_horse_01_head_000_c0_991_ab",
    0x238B4185: "mp_horse_01_hand_000_c0_833_ab",
    0x25988B48: "p_c_horse_01_head_000_c0_999_ab",
    0x25D15B14: "p_c_horse_01_teef_000_c0_000_ab",
    0x25ECEC41: "p_c_horse_01_hand_000_c0_882_ab",
    0x271370A2: "mp_horse_01_head_000_c0_844_ab",
    0x27E50F8A: "mp_horse_01_head_000_c0_835_ab",
    0x27E986CB: "p_c_horse_01_hand_000_c0_889_ab",
    0x285EDED4: "p_c_horse_01_head_000_c0_864_ab",
    0x29476B58: "p_c_horse_01_uppr_002_c0_000_ab",
    0x29AF9130: "p_c_horse_01_head_000_c0_993_ab",
    0x2B3497F1: "p_c_horse_01_head_000_c0_889_ab",
    0x2B5BCC01: "p_c_horse_01_head_000_c0_891_ab",
    0x2B5C884B: "p_c_horse_01_head_000_c0_897_ab",
    0x2B668DB6: "mp_horse_01_head_000_c0_838_ab",
    0x2BFA9501: "p_c_horse_01_head_000_c0_989_ab",
    0x2C25780A: "mp_horse_01_head_000_c0_840_ab",
    0x2C302313: "p_c_horse_01_slvl_001_c0_000_ab",
    0x3474D1F0: "p_c_horse_01_hair_000_c0_998_ab",
    0x34E9D503: "mp_horse_01_head_000_c0_842_ab",
    0x3510E2FD: "mp_horse_01_head_000_c0_832_ab",
    0x368EE8A6: "mp_horse_01_hand_000_c0_824_ab",
    0x371844D0: "mp_horse_01_hair_000_c0_000_ab",
    0x37CA5A02: "mp_horse_01_hand_000_c0_840_ab",
    0x3C1AC078: "p_c_horse_01_head_000_c0_871_ab",
    0x3D20794F: "p_c_horse_01_hand_000_c0_873_ab",
    0x3DC33635: "mp_horse_01_hand_000_c0_823_ab",
    0x3E88A345: "p_c_horse_01_head_000_c0_895_ab",
    0x4074852B: "mp_horse_01_hand_000_c0_839_ab",
    0x41C38EEF: "p_c_horse_01_hand_000_c0_995_ab",
    0x423B60A8: "mp_horse_01_hair_000_c0_999_ab",
    0x42589694: "p_c_horse_01_head_000_c0_988_ab",
    0x42D81168: "p_c_horse_01_head_000_c0_875_ab",
    0x430D94C3: "p_c_horse_01_hair_000_c0_000_ab",
    0x44340895: "horn_hr1_003_c0_000_ab",
    0x4578056F: "p_c_horse_01_hair_001_c0_999_ab",
    0x484A6E9F: "mp_horse_01_head_000_c0_833_ab",
    0x48C1ABAB: "mp_horse_01_head_000_c0_841_ab",
    0x490CD77A: "p_c_horse_01_hand_000_c0_871_ab",
    0x491779BF: "p_c_horse_01_hand_000_c0_866_ab",
    0x4966129B: "p_c_horse_01_head_000_c0_863_ab",
    0x49F39E30: "mp_horse_01_hand_000_c0_836_ab",
    0x4B280731: "p_c_horse_01_hand_000_c0_877_ab",
    0x4D96CB3A: "p_c_horse_01_hand_000_c0_884_ab",
    0x56F05A18: "mp_horse_01_hand_000_c0_827_ab",
    0x57D9CCAB: "stirrup_hr1_008_c0_000_ab",
    0x586F5DD0: "p_c_horse_01_head_000_c0_998_ab",
    0x58AA1B78: "p_c_horse_01_belt_001_c0_000_ab",
    0x5ADA1481: "p_c_horse_01_head_000_c0_861_ab",
    0x5B3F6EEE: "p_c_horse_01_head_000_c0_887_ab",
    0x5CFE74AF: "mp_horse_01_head_000_c0_843_ab",
    0x5E2EC866: "p_c_horse_01_head_000_c0_888_ab",
    0x60C257DA: "p_c_horse_01_feet_001_c0_000_ab",
    0x61F86FD2: "p_c_horse_01_hand_000_c0_875_ab",
    0x62056C46: "mp_horse_01_hand_000_c0_835_ab",
    0x627031A6: "p_c_horse_01_hand_000_c0_862_ab",
    0x64AFA858: "p_c_horse_01_hand_000_c0_998_ab",
    0x654E2BA0: "p_c_horse_01_hand_000_c0_861_ab",
    0x65BC843D: "p_c_horse_01_hand_000_c0_888_ab",
    0x65C84192: "mp_horse_01_head_000_c0_836_ab",
    0x660D197B: "p_c_horse_01_hair_000_c0_997_ab",
    0x67E37518: "mp_horse_01_hand_000_c0_822_ab",
    0x6A0C1F5F: "p_c_horse_01_head_000_c0_996_ab",
    0x6A89C927: "mp_horse_01_head_000_c0_826_ab",
    0x6ED7F8B9: "p_c_horse_01_hair_000_c0_996_ab",
    0x6FEC5EB3: "p_c_horse_01_hand_000_c0_886_ab",
    0x717A8CCC: "p_c_horse_01_head_000_c0_878_ab",
    0x718AC124: "mp_horse_01_hair_000_c0_998_ab",
    0x74791D3A: "p_c_horse_01_teef_000_c0_001_ab",
    0x747E88A0: "mp_horse_01_hand_000_c0_828_ab",
    0x748FF64F: "p_c_horse_01_head_000_c0_879_ab",
    0x76A9266B: "mp_horse_01_hand_000_c0_831_ab",
    0x771E1CD1: "mp_horse_01_hand_000_c0_844_ab",
    0x775C04DD: "p_c_horse_01_head_000_c0_096_ab",
    0x795DE09C: "p_c_horse_01_hand_000_c0_885_ab",
    0x7C06683F: "p_c_horse_01_head_000_c0_867_ab",
    0x7E7C8956: "p_c_horse_01_head_000_c0_992_ab",
    0x817CBEF3: "p_c_horse_01_hand_000_c0_863_ab",
    0x82818578: "p_c_horse_01_hand_000_c0_993_ab",
    0x834701C7: "mp_horse_01_head_000_c0_827_ab",
    0x8BFF94C7: "p_c_horse_01_hand_000_c0_890_ab",
    0x8E84C5DF: "mp_horse_01_hand_000_c0_838_ab",
    0x8FEAAEE4: "p_c_horse_01_hand_000_c0_865_ab",
    0x91CA4FB2: "p_c_horse_01_hand_000_c0_988_ab",
    0x95D39DD2: "p_c_horse_01_head_000_c0_877_ab",
    0x95EF1E85: "mp_horse_01_head_000_c0_831_ab",
    0x9A6178C1: "p_c_horse_01_head_000_c0_994_ab",
    0x9B092F21: "p_c_horse_01_hand_000_c0_887_ab",
    0x9B663734: "mp_horse_01_hair_000_c0_996_ab",
    0x9BB5E26B: "p_c_horse_01_head_000_c0_995_ab",
    0x9CB0B8F3: "p_c_horse_01_head_000_c0_870_ab",
    0x9ED04A50: "p_c_horse_01_head_000_c0_884_ab",
    0x9EDC783A: "p_c_horse_01_hand_000_c0_099_ab",
    0xA0992057: "p_c_horse_01_hand_000_c0_898_ab",
    0xA178B8E5: "p_c_horse_01_mis2_000_c0_000_ab",
    0xA1AC42C9: "p_c_horse_01_hand_000_c0_897_ab",
    0xA3A799BD: "p_c_horse_01_hand_000_c0_991_ab",
    0xA3A7C052: "p_c_horse_01_acc2_001_c1_993_ab",
    0xA4D9B97A: "p_c_horse_01_head_000_c0_866_ab",
    0xA687C792: "p_c_horse_01_head_000_c0_097_ab",
    0xA855E542: "p_c_horse_01_head_000_c0_883_ab",
    0xAA4C7C37: "mp_horse_01_hand_000_c0_825_ab",
    0xACEDD34D: "p_c_horse_01_hand_000_c0_870_ab",
    0xAF566330: "mp_horse_01_head_000_c0_825_ab",
    0xB0117436: "mp_horse_01_hair_001_c0_999_ab",
    0xB2D7B85B: "mp_horse_01_hand_000_c0_829_ab",
    0xB3252C13: "mp_horse_01_head_000_c0_834_ab",
    0xB5D143FA: "p_c_horse_01_head_000_c0_885_ab",
    0xB69AF681: "p_c_horse_01_hand_000_c0_097_ab",
    0xB87E3B6B: "mp_horse_01_hand_000_c0_834_ab",
    0xBB5B60B1: "p_c_horse_01_hand_000_c0_868_ab",
    0xBB8EBE02: "p_c_horse_01_head_000_c0_881_ab",
    0xBC4FE5D5: "p_c_horse_01_head_000_c0_868_ab",
    0xBD1E8996: "p_c_horse_01_head_000_c0_898_ab",
    0xC04AC71E: "p_c_horse_01_head_000_c0_899_ab",
    0xC43F7F29: "p_c_horse_01_hand_000_c0_994_ab",
    0xC81C1AAC: "p_c_horse_01_hand_000_c0_992_ab",
    0xCCDD64CE: "p_c_horse_01_hand_000_c0_894_ab",
    0xCE6266D1: "mp_horse_01_hand_000_c0_826_ab",
    0xD14D40AE: "p_c_horse_01_hand_000_c0_899_ab",
    0xD24DD595: "p_c_horse_01_hand_000_c0_879_ab",
    0xD59050DF: "p_c_horse_01_head_000_c0_882_ab",
    0xD5F5C9AF: "mp_horse_01_head_000_c0_823_ab",
    0xD6E0FE5F: "p_c_horse_01_tas2_000_c0_000_ab",
    0xD7DDE492: "mp_horse_01_head_000_c0_830_ab",
    0xDF3B429C: "p_c_horse_01_hand_000_c0_891_ab",
    0xE2ADB21F: "p_c_horse_01_accs_001_c3_999_ab",
    0xE364BEC6: "mp_horse_01_hand_000_c0_830_ab",
    0xE4DF2CD8: "mp_horse_01_hair_000_c0_994_ab",
    0xE52DFBC0: "p_c_horse_01_head_000_c0_099_ab",
    0xE658DD74: "p_c_horse_01_hand_000_c0_883_ab",
    0xE7D5E9DD: "p_c_horse_01_head_000_c0_886_ab",
    0xE98F788A: "p_c_horse_01_hand_000_c0_878_ab",
    0xE9925F46: "p_c_horse_01_hand_000_c0_997_ab",
    0xEBFD74C8: "p_c_horse_01_head_000_c0_896_ab",
    0xED2E8A67: "p_c_horse_01_hand_000_c0_867_ab",
    0xF2958374: "mp_horse_01_head_000_c0_828_ab",
    0xF2FB01FE: "mp_horse_01_hand_000_c0_837_ab",
    0xF7196854: "mp_horse_01_hand_000_c0_843_ab",
    0xF84E8ECC: "p_c_horse_01_head_000_c0_862_ab",
    0xF915D39F: "p_c_horse_01_head_000_c0_890_ab",
    0xFA0208CC: "mp_horse_01_hair_000_c0_997_ab",
    0xFB721189: "mp_horse_01_hand_000_c0_841_ab",
    0xFC978E5D: "p_c_horse_01_eyes_000_c0_000_ab",
    0xFFFAE34A: "p_c_horse_01_hand_000_c0_989_ab",
}



class XmlNode(object):

    __slots__ = ("tag", "start", "open_end", "close_start", "end",
                 "attrs", "children", "parent", "source")

    def __init__(self, tag, start, parent=None):
        self.tag = tag
        self.start = start
        self.open_end = 0
        self.close_start = 0
        self.end = 0

        self.attrs = {}
        self.children = []
        self.parent = parent
        self.source = ""

    def child(self, tag):
        for c in self.children:
            if c.tag == tag:
                return c
        return None

    def find_all(self, tag):
        return [c for c in self.children if c.tag == tag]

    def text(self):
        if self.close_start <= self.open_end:
            return ""
        return self.source[self.open_end:self.close_start].strip()

    def attr_value(self, name, default=None):
        entry = self.attrs.get(name)
        return entry[0] if entry else default

    def attr_span(self, name):
        entry = self.attrs.get(name)
        return (entry[1], entry[2]) if entry else None


def parse_xml_spans(text):
    length = len(text)
    index = 0
    stack = []
    root = None

    while index < length:
        lt = text.find("<", index)
        if lt < 0:
            break
        if text.startswith("<!--", lt):
            end = text.find("-->", lt)
            index = length if end < 0 else end + 3
            continue
        if text.startswith("<![CDATA[", lt):
            end = text.find("]]>", lt)
            index = length if end < 0 else end + 3
            continue
        if text.startswith("<?", lt):
            end = text.find("?>", lt)
            index = length if end < 0 else end + 2
            continue
        if text.startswith("<!", lt):
            end = text.find(">", lt)
            index = length if end < 0 else end + 1
            continue
        if text.startswith("</", lt):
            gt = text.find(">", lt)
            if gt < 0:
                break
            if stack:
                node = stack.pop()
                node.close_start = lt
                node.end = gt + 1
            index = gt + 1
            continue

        cursor = lt + 1
        match = re.match(r"[^\s/>]+", text[cursor:])
        if not match:
            index = lt + 1
            continue
        tag = match.group(0)
        cursor += len(tag)

        attrs = {}
        self_closing = False
        while cursor < length:
            while cursor < length and text[cursor].isspace():
                cursor += 1
            if cursor >= length:
                break
            char = text[cursor]
            if char == "/":
                self_closing = True
                cursor += 1
                continue
            if char == ">":
                cursor += 1
                break
            attr_match = re.match(r"[^\s=/>]+", text[cursor:])
            if not attr_match:
                cursor += 1
                continue
            attr_name = attr_match.group(0)
            cursor += len(attr_name)
            probe = cursor
            while probe < length and text[probe].isspace():
                probe += 1
            if probe < length and text[probe] == "=":
                cursor = probe + 1
                while cursor < length and text[cursor].isspace():
                    cursor += 1
                if cursor < length and text[cursor] in "\"'":
                    quote = text[cursor]
                    close = text.find(quote, cursor + 1)
                    if close < 0:
                        close = length - 1
                    attrs[attr_name] = (text[cursor + 1:close], cursor + 1, close)
                    cursor = close + 1
                else:
                    value_match = re.match(r"[^\s>]+", text[cursor:])
                    raw = value_match.group(0) if value_match else ""
                    attrs[attr_name] = (raw, cursor, cursor + len(raw))
                    cursor += len(raw)
            else:
                attrs[attr_name] = ("", cursor, cursor)

        parent = stack[-1] if stack else None
        node = XmlNode(tag, lt, parent)
        node.open_end = cursor
        node.attrs = attrs
        if parent is not None:
            parent.children.append(node)
        elif root is None:
            root = node
        if self_closing:
            node.close_start = cursor
            node.end = cursor
        else:
            stack.append(node)
        index = cursor

    _attach_source(root, text)
    return root


def _attach_source(node, text):
    if node is None:
        return
    node.source = text
    for child in node.children:
        _attach_source(child, text)



class TextureSlot(object):

    __slots__ = ("index", "lod", "drawable", "albedo", "normal", "material",
                 "palette", "tints", "probability", "tint_nodes", "part",
                 "drawable_node", "albedo_node", "palette_node")

    def __init__(self, index, lod):
        self.index = index
        self.lod = lod
        self.drawable = ""
        self.albedo = ""
        self.normal = ""
        self.material = ""
        self.palette = ""
        self.tints = (0, 0, 0)
        self.probability = 255
        self.tint_nodes = ()
        self.part = Tag.PART_OTHER
        self.drawable_node = None
        self.albedo_node = None
        self.palette_node = None

    @property
    def drawable_int(self):
        try:
            return int(self.drawable, 16)
        except (TypeError, ValueError):
            return 0

    @property
    def albedo_int(self):
        try:
            return int(self.albedo, 16)
        except (TypeError, ValueError):
            return 0

    @property
    def asset_name(self):
        info = DRAWABLE_INFO.get(self.drawable_int)
        return info[1] if info else ""


class YmtDocument(object):

    def __init__(self, path):
        self.path = path
        self.filename = os.path.basename(path)
        with open(path, "rb") as handle:
            raw = handle.read()
        self.has_bom = raw.startswith(b"\xef\xbb\xbf")
        self.text = raw.decode("utf-8-sig")
        self.root = parse_xml_spans(self.text)
        self.lods = []
        self.slots = []
        self.scale_nodes = []
        self.offset_nodes = []
        self.redness_nodes = []
        self._parse()


    def _parse(self):
        outfits = self.root.child(Tag.OUTFITS) if self.root else None
        if outfits is None:
            return
        for lod_index, outfit in enumerate(outfits.find_all("Item")):
            self.lods.append(outfit)
            node = outfit.child(Tag.SCALE)
            if node is not None:
                self.scale_nodes.append((lod_index, node))
            node = outfit.child(Tag.SCALE_RANDOM_OFFSET)
            if node is not None:
                self.offset_nodes.append((lod_index, node))
            node = outfit.child(Tag.EYE_REDNESS)
            if node is not None:
                self.redness_nodes.append((lod_index, node))
            assets = outfit.child(Tag.EXPLICIT_ASSETS)
            if assets is None:
                continue
            for slot_index, item in enumerate(assets.find_all("Item")):
                slot = TextureSlot(slot_index, lod_index)
                slot.drawable_node = item.child(Tag.DRAWABLE)
                slot.albedo_node = item.child(Tag.ALBEDO)
                slot.drawable = slot.drawable_node.text() if slot.drawable_node is not None else ""
                slot.albedo = slot.albedo_node.text() if slot.albedo_node is not None else ""
                slot.normal = self._text_of(item, Tag.NORMAL)
                slot.material = self._text_of(item, Tag.MATERIAL)
                slot.palette_node = item.child(Tag.PALETTE)
                slot.palette = self._text_of(item, Tag.PALETTE)
                slot.tint_nodes = self._tint_nodes(item)
                slot.tints = self._tint_values(slot.tint_nodes)
                prob = item.child(Tag.PROBABILITY)
                if prob is not None:
                    try:
                        slot.probability = int(float(prob.attr_value("value", "255")))
                    except ValueError:
                        slot.probability = 255
                info = DRAWABLE_INFO.get(slot.drawable_int)
                slot.part = info[0] if info else Tag.PART_OTHER
                self.slots.append(slot)

    @staticmethod
    def _text_of(item, tag):
        node = item.child(tag)
        return node.text() if node is not None else ""

    @staticmethod
    def _tint_nodes(item):
        named = [c for c in item.children if c.tag in (Tag.TINT0, Tag.TINT1, Tag.TINT2)]
        if len(named) == 3:
            return tuple(named)
        fallback = []
        for child in item.children:
            if child.tag == Tag.PROBABILITY:
                continue
            raw = child.attr_value("value")
            if raw is not None and re.match(r"^\d+$", raw.strip()):
                fallback.append(child)
        return tuple(fallback[:3])

    @staticmethod
    def _tint_values(nodes):
        values = [0, 0, 0]
        for index, node in enumerate(nodes[:3]):
            try:
                values[index] = max(0, min(255, int(float(node.attr_value("value", "0")))))
            except (TypeError, ValueError):
                values[index] = 0
        return tuple(values)


    @property
    def lod_count(self):
        return len(self.lods)

    @property
    def scale(self):
        for _, node in self.scale_nodes:
            try:
                return float(node.attr_value("value", "1.0"))
            except (TypeError, ValueError):
                return 1.0
        return 1.0

    @property
    def scale_random_offset(self):
        for _, node in self.offset_nodes:
            try:
                return float(node.attr_value("value", "0.0"))
            except (TypeError, ValueError):
                return 0.0
        return 0.0

    @property
    def eye_redness(self):
        for _, node in self.redness_nodes:
            try:
                return float(node.attr_value("value", "0.0"))
            except (TypeError, ValueError):
                return 0.0
        return 0.0

    def slots_for_lod(self, lod=0):
        return [s for s in self.slots if s.lod == lod]


    def _clone_item(self, template, values):
        raw = self.text[template.start:template.end]
        edits = []
        for child in template.children:
            new_value = values.get(child.tag)
            if new_value is None:
                continue
            span = child.attr_span("value")
            if span:

                edits.append((span[0], span[1], new_value))
            elif child.close_start <= child.open_end:

                edits.append((child.start, child.end,
                              "<%s>%s</%s>" % (child.tag, new_value, child.tag)))
            else:
                edits.append((child.open_end, child.close_start, new_value))
        edits.sort(key=lambda item: item[0], reverse=True)
        for start, end, replacement in edits:
            raw = raw[:start - template.start] + replacement + raw[end - template.start:]
        return raw


    def render(self, scale=None, tint_changes=None, texture_changes=None,
               palette_changes=None, offset=None, redness=None,
               scale_all_lods=True):
        edits = []
        if scale is not None:
            targets = self.scale_nodes if scale_all_lods else self.scale_nodes[:1]
            for _, node in targets:
                span = node.attr_span("value")
                if span:
                    edits.append((span[0], span[1], "%.8f" % scale))
        if offset is not None:
            for _, node in self.offset_nodes:
                span = node.attr_span("value")
                if span:
                    edits.append((span[0], span[1], "%.8f" % offset))
        if redness is not None:
            for _, node in self.redness_nodes:
                span = node.attr_span("value")
                if span:
                    edits.append((span[0], span[1], "%.8f" % redness))
        if tint_changes:
            for slot in self.slots:
                tints = tint_changes.get(slot.drawable)
                if tints is None:
                    continue
                for node, value in zip(slot.tint_nodes, tints):
                    span = node.attr_span("value")
                    if span:
                        edits.append((span[0], span[1], str(int(value))))
        if texture_changes:
            for slot in self.slots:
                new_hash = texture_changes.get(slot.drawable)
                if new_hash is None or slot.albedo_node is None:
                    continue

                start = slot.albedo_node.open_end
                end = slot.albedo_node.close_start
                if end > start:
                    edits.append((start, end, new_hash))
        if palette_changes:
            for slot in self.slots:
                new_hash = palette_changes.get(slot.drawable)
                if new_hash is None:
                    continue
                node = slot.palette_node
                if node is None:
                    continue
                if new_hash:
                    replacement = "<%s>%s</%s>" % (node.tag, new_hash, node.tag)
                else:
                    replacement = "<%s/>" % node.tag
                edits.append((node.start, node.end, replacement))

        edits.sort(key=lambda item: item[0], reverse=True)
        out = self.text
        for start, end, replacement in edits:
            out = out[:start] + replacement + out[end:]
        return out

    def write_bytes(self, content):
        data = content.encode("utf-8")
        if self.has_bom:
            data = b"\xef\xbb\xbf" + data
        return data



class HorseRecord(object):

    def __init__(self, document):
        self.document = document
        self.filename = document.filename
        self.stem = os.path.splitext(document.filename)[0]
        name = HORSE_NAMES.get(self.stem) or self._fallback_name()
        self.is_gang = "_gang_" in self.stem
        self.pretty = ("(Gang) " + name) if self.is_gang else name
        self.breed, _, self.coat = name.partition(" - ")
        self.scale = document.scale
        self.vanilla_scale = document.scale
        self.vanilla_offset = document.scale_random_offset
        self.vanilla_redness = document.eye_redness
        self.eye_redness = document.eye_redness
        self.lod_count = document.lod_count
        self.slots = document.slots_for_lod(0)
        self.vanilla_tints = {s.drawable: s.tints for s in self.slots}
        self.vanilla_textures = {s.drawable: s.albedo for s in self.slots}
        self.vanilla_palettes = {s.drawable: s.palette for s in self.slots}
        self.eye_colour = self._detect_eye_colour()
        self.vanilla_eye_colour = self.eye_colour

    def _detect_eye_colour(self):
        for slot in self.slots:
            if slot.part != Tag.PART_EYES:
                continue
            for index, (texture, _label) in enumerate(EYE_TEXTURES):
                if slot.albedo_int == texture:
                    return index
            return 0
        return 0

    def _fallback_name(self):
        name = self.stem
        for prefix in ("a_c_horse_", "a_c_horsemule", "a_c_"):
            if name.startswith(prefix):
                name = name[len(prefix):]
                break
        name = name.lstrip("_")
        breed, _, coat = name.partition("_")
        breed = breed.replace("_", " ").title()
        coat = coat.replace("_", " ").title()
        return ("%s - %s" % (breed, coat)).strip(" -") if coat else breed

    @property
    def display_name(self):
        return self.pretty

    @property
    def search_text(self):
        return (self.stem + " " + self.display_name).lower()


def load_database(vanilla_dir):
    records = []
    if not os.path.isdir(vanilla_dir):
        return records
    for path in sorted(glob.glob(os.path.join(vanilla_dir, "*.ymt"))):
        try:
            records.append(HorseRecord(YmtDocument(path)))
        except Exception as exc:
            sys.stderr.write("Could not parse %s: %s\n" % (path, exc))
    return records


def build_coat_catalogue(records):
    catalogue = {}
    users = {}
    for record in records:
        for slot in record.slots:
            if slot.part not in (Tag.PART_BODY, Tag.PART_HEAD):
                continue
            body = slot.albedo_int
            if not body:
                continue
            entry = COAT_PAIRS.get(body)
            if entry is None:
                continue
            label, other = entry
            if body in catalogue:
                continue
            catalogue[body] = (label, other)
    for record in records:
        for slot in record.slots:
            if slot.part == Tag.PART_BODY and slot.albedo_int in catalogue:
                users.setdefault(slot.albedo_int, []).append(record.display_name)
    return catalogue, users


def build_tint_catalogue(records):
    catalogue = {}
    for record in records:
        for slot in record.slots:
            key = (slot.part, slot.drawable)
            catalogue.setdefault(key, {})
            catalogue[key].setdefault(slot.tints, []).append(record.display_name)
    return catalogue


def build_scale_catalogue(records):
    raw = {}
    for record in records:
        key = round(record.vanilla_scale, 5)
        breeds = raw.setdefault(key, {})
        breeds.setdefault(record.breed, set()).add(record.is_gang)
    catalogue = {}
    for key, breeds in raw.items():
        plain = []
        gang = []
        for breed, gang_flags in breeds.items():
            if gang_flags == {True}:
                gang.append("Gang - %s" % breed)
            else:
                plain.append(breed)
        catalogue[key] = sorted(plain) + sorted(gang)
    return catalogue



EYE_TEXTURES = (
    (0x25D15B14, "Brown"),
    (0x74791D3A, "Blue"),
)





BG = "#1b1d21"
BG_PANEL = "#24272d"
BG_INPUT = "#2c3037"
FG = "#e6e6e8"
FG_DIM = "#9aa0a8"
ACCENT = "#e11024"
ACCENT_SOFT = "#3a2226"
BORDER = "#383d45"


class HorseManagerApp(tk.Tk):
    def __init__(self, vanilla_dir=VANILLA_DIR, modded_dir=MODDED_DIR):
        super(HorseManagerApp, self).__init__()
        self.title("%s  v%s" % (APP_NAME, APP_VERSION))
        self.geometry("1240x820")
        self.minsize(1020, 660)
        self.configure(background=BG)

        self.vanilla_dir = vanilla_dir
        self.modded_dir = modded_dir
        self.records = []
        self.record_by_id = {}
        self.current = None
        self.dirty = False
        self.coat_catalogue = {}
        self.coat_users = {}
        self.tint_catalogue = {}
        self.scale_catalogue = {}

        self._build_style()
        self._build_ui()
        self._bind_keys()
        self.state("zoomed")
        self.after(60, self._initial_load)


    def _build_style(self):
        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        style.configure(".", background=BG, foreground=FG,
                        fieldbackground=BG_INPUT, bordercolor=BORDER,
                        font=("Segoe UI", 8))
        style.configure("TFrame", background=BG)
        style.configure("Panel.TFrame", background=BG_PANEL)
        style.configure("TLabel", background=BG, foreground=FG)
        style.configure("Panel.TLabel", background=BG_PANEL, foreground=FG)
        style.configure("Dim.TLabel", background=BG_PANEL, foreground=FG_DIM,
                        font=("Segoe UI", 8))
        style.configure("Title.TLabel", background=BG, foreground=FG,
                        font=("Segoe UI", 15, "bold"))
        style.configure("Sub.TLabel", background=BG, foreground=FG_DIM,
                        font=("Segoe UI", 8))
        style.configure("Head.TLabel", background=BG_PANEL, foreground=FG,
                        font=("Segoe UI", 9, "bold"))
        style.configure("TButton", background=BG_INPUT, foreground=FG,
                        bordercolor=BORDER, focuscolor=BG_INPUT,
                        font=("Segoe UI", 8), padding=(6, 2))
        style.map("TButton",
                  background=[("active", "#3a3f48"), ("pressed", "#464c57")])
        style.configure("Accent.TButton", background=ACCENT, foreground="#ffffff",
                        font=("Segoe UI", 9, "bold"), bordercolor=ACCENT,
                        padding=(6, 2))
        style.map("Accent.TButton",
                  background=[("active", "#f51a30"), ("pressed", "#c00d1e")])
        style.configure("TEntry", fieldbackground=BG_INPUT, foreground=FG,
                        insertcolor=FG, bordercolor=BORDER)
        style.configure("TSpinbox", fieldbackground=BG_INPUT, foreground=FG,
                        background=BG_INPUT, bordercolor=BORDER,
                        arrowcolor=FG, arrowsize=12)
        style.configure("TCheckbutton", background=BG_PANEL, foreground=FG)
        style.configure("TCombobox", fieldbackground=BG_INPUT, background=BG_INPUT,
                        foreground=FG, bordercolor=BORDER, arrowcolor=FG,
                        selectbackground=BG_INPUT, selectforeground=FG,
                        insertcolor=FG)
        style.map("TCombobox",
                  fieldbackground=[("readonly", BG_INPUT)],
                  background=[("readonly", BG_INPUT), ("active", BG_INPUT)],
                  foreground=[("readonly", FG)],
                  selectbackground=[("readonly", BG_INPUT)],
                  selectforeground=[("readonly", FG)],
                  bordercolor=[("focus", ACCENT)])
        style.configure("Horizontal.TScale", background=BG_PANEL,
                        troughcolor=BG_INPUT, bordercolor=BORDER,
                        lightcolor=BG_INPUT, darkcolor=BG_INPUT)
        style.configure("Treeview", background=BG_INPUT, fieldbackground=BG_INPUT,
                        foreground=FG, bordercolor=BORDER, rowheight=19,
                        font=("Segoe UI", 8))
        style.configure("Treeview.Heading", background=BG_PANEL, foreground=FG,
                        font=("Segoe UI", 8, "bold"))
        style.map("Treeview", background=[("selected", ACCENT_SOFT)])
        style.configure("TNotebook", background=BG, bordercolor=BORDER)
        style.configure("TNotebook.Tab", background=BG_PANEL, foreground=FG_DIM,
                        padding=(12, 6))
        style.map("TNotebook.Tab",
                  background=[("selected", BG)], foreground=[("selected", FG)])


    def _build_ui(self):
        header = tk.Frame(self, bg=BG)
        header.pack(side="top", fill="x", padx=18, pady=(14, 6))
        ttk.Label(header, text=APP_NAME, style="Title.TLabel").pack(side="left")
        ttk.Label(header, text="RDR2 horse metadata (.ymt) editor   -   "
                               "vanilla is read-only, output goes to Modded",
                  style="Sub.TLabel").pack(side="left", padx=(14, 0))
        self.header_status = tk.Label(header, text="", bg=BG, fg=FG_DIM,
                                      font=("Consolas", 9))
        self.header_status.pack(side="right")

        body = tk.Frame(self, bg=BG)
        body.pack(side="top", fill="both", expand=True, padx=18, pady=(4, 6))
        body.columnconfigure(0, weight=0, minsize=self.list_panel_width())
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)
        self.body = body

        self._build_list_panel(body)
        self._build_detail_panel(body)

        footer = tk.Frame(self, bg=BG_PANEL)
        footer.pack(side="bottom", fill="x")
        self.log = tk.Text(footer, height=3, bg=BG_PANEL, fg=FG_DIM, bd=0,
                           font=("Consolas", 9), insertbackground=FG,
                           highlightthickness=0, padx=10, pady=6)
        self.log.pack(side="left", fill="both", expand=True)
        self.log_state = tk.Label(footer, text="", bg=BG_PANEL, fg=FG_DIM,
                                  font=("Segoe UI", 9), padx=12)
        self.log_state.pack(side="right")

    def _build_list_panel(self, parent):
        panel = tk.Frame(parent, bg=BG_PANEL, highlightthickness=1,
                         highlightbackground=BORDER)
        panel.grid(row=0, column=0, sticky="nsew", padx=(0, 12))
        panel.rowconfigure(3, weight=1)
        panel.columnconfigure(0, weight=1)

        ttk.Label(panel, text="VANILLA DATABASE", style="Head.TLabel").grid(
            row=0, column=0, sticky="w", padx=12, pady=(10, 2))
        self.vanilla_path_label = tk.Label(panel, text="", bg=BG_PANEL, fg=FG_DIM,
                                           font=("Consolas", 8), anchor="w",
                                           justify="left", wraplength=340)
        self.vanilla_path_label.grid(row=1, column=0, sticky="ew", padx=12)

        search_row = tk.Frame(panel, bg=BG_PANEL)
        search_row.grid(row=2, column=0, sticky="ew", padx=12, pady=(10, 6))
        search_row.columnconfigure(0, weight=1)
        self.search_var = tk.StringVar()
        self.search_entry = ttk.Entry(search_row, textvariable=self.search_var)
        self.search_entry.grid(row=0, column=0, columnspan=2, sticky="ew")
        self.breed_var = tk.StringVar(value="All breeds")
        self.breed_box = ttk.Combobox(search_row, textvariable=self.breed_var,
                                      state="readonly")
        self.breed_box.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(6, 0))
        self.breed_box.bind("<<ComboboxSelected>>", lambda e: self._refresh_list())

        tree_frame = tk.Frame(panel, bg=BG_PANEL)
        tree_frame.grid(row=3, column=0, sticky="nsew", padx=(12, 8))
        tree_frame.rowconfigure(0, weight=1)
        tree_frame.columnconfigure(0, weight=1)

        columns = ("scale",)
        self.tree = ttk.Treeview(tree_frame, columns=columns,
                                  show="tree headings", selectmode="browse")
        self.tree.heading("#0", text="Horse")
        self.tree.heading("scale", text="Scale")
        self.tree.column("#0", width=250, minwidth=160, stretch=True)
        self.tree.column("scale", width=55, minwidth=52, stretch=False,
                         anchor="e")
        self.tree.grid(row=0, column=0, sticky="nsew")
        scroll = ttk.Scrollbar(tree_frame, orient="vertical", command=self.tree.yview)
        scroll.grid(row=0, column=1, sticky="ns")
        self.tree.configure(yscrollcommand=scroll.set)
        self.tree.bind("<<TreeviewSelect>>", self._on_select)

        bottom = tk.Frame(panel, bg=BG_PANEL)
        bottom.grid(row=4, column=0, sticky="ew", padx=12, pady=(8, 12))
        ttk.Button(bottom, text="Refresh database",
                   command=self._initial_load).pack(side="left")
        ttk.Button(bottom, text="Open Modded folder",
                   command=self._open_modded).pack(side="left", padx=(6, 0))

    def _build_detail_panel(self, parent):
        panel = tk.Frame(parent, bg=BG)
        panel.grid(row=0, column=1, sticky="nsew")
        panel.rowconfigure(1, weight=1)
        panel.columnconfigure(0, weight=1)

        head = tk.Frame(panel, bg=BG)
        head.grid(row=0, column=0, sticky="ew")
        self.detail_title = ttk.Label(head, text="Select a horse", style="Title.TLabel")
        self.detail_title.pack(side="left")
        self.detail_sub = ttk.Label(head, text="", style="Sub.TLabel")
        self.detail_sub.pack(side="left", padx=(12, 0))

        notebook = ttk.Notebook(panel)
        notebook.grid(row=1, column=0, sticky="nsew", pady=(8, 0))
        self.notebook = notebook

        self._build_appearance_tab(notebook)
        self._build_save_tab(notebook)


    def _build_appearance_tab(self, notebook):
        tab = self._scrollable_tab(notebook, "  Appearance  ")
        tab.columnconfigure(0, weight=1)


        scale_card = self._card(tab, "HORSE SCALE", row=0, sticky="ew",
                                padx=14, pady=(12, 0))
        scale_card.columnconfigure(1, weight=1)
        self.scale_var = tk.StringVar(value="1.000000")
        self.scale_entry = ttk.Entry(scale_card, textvariable=self.scale_var, width=14)
        self.scale_entry.grid(row=0, column=1, sticky="w")
        self.scale_entry.bind("<Return>", lambda e: self._apply_scale())
        self.scale_entry.bind("<FocusOut>", lambda e: self._apply_scale())
        ttk.Button(scale_card, text="Reset", width=8,
                   command=self._reset_scale).grid(row=0, column=3, padx=(10, 0))
        ttk.Button(scale_card, text="Apply", width=8,
                   command=self._apply_scale).grid(row=0, column=4, padx=(6, 0))

        scale_row = tk.Frame(scale_card, bg=BG_PANEL)
        scale_row.grid(row=1, column=0, columnspan=5, sticky="ew", pady=(6, 0))
        scale_row.columnconfigure(1, weight=1)
        ttk.Label(scale_row, text="Copy scale from", style="Dim.TLabel").grid(
            row=0, column=0, sticky="w")
        self.scale_preset_var = tk.StringVar(value="- pick a horse -")
        self.scale_preset_box = ttk.Combobox(scale_row,
                                             textvariable=self.scale_preset_var,
                                             state="readonly")
        self.scale_preset_box.grid(row=0, column=1, sticky="ew", padx=(8, 0))
        self.scale_preset_box.bind("<<ComboboxSelected>>", self._on_scale_preset)


        self.part_cards = {}
        for index, part in enumerate((Tag.PART_BODY, Tag.PART_HEAD, Tag.PART_EYES)):
            card = self._card(tab, part.upper(), row=index + 1, sticky="ew",
                              padx=14, pady=(12, 0))


            self._build_part_card(card, part)


        slot_card = self._card(tab, "TEXTURE SLOTS", row=4, sticky="nsew",
                               padx=14, pady=(10, 0))
        slot_card.rowconfigure(1, weight=1)
        slot_card.columnconfigure(0, weight=1)
        ttk.Label(slot_card,
                  text="Every explicit asset of the model. tint0 / tint1 / tint2 "
                       "are the rows of the tint palette (255 switches a channel off).",
                  style="Dim.TLabel").grid(row=0, column=0, sticky="w", pady=(0, 4))
        slot_table = tk.Frame(slot_card, bg=BG_PANEL)
        slot_table.grid(row=1, column=0, sticky="nsew")
        slot_table.rowconfigure(0, weight=1)
        slot_table.columnconfigure(0, weight=1)

        columns = ("part", "drawable", "albedo", "t0", "t1", "t2")
        self.slot_tree = ttk.Treeview(slot_table, columns=columns,
                                      show="headings", selectmode="browse", height=8)
        for name, label, width in (("part", "Part", 110),
                                   ("drawable", "Drawable", 105),
                                   ("albedo", "Texture", 150),
                                   ("t0", "tint0", 46), ("t1", "tint1", 46),
                                   ("t2", "tint2", 46)):
            self.slot_tree.heading(name, text=label)
            self.slot_tree.column(name, width=width, anchor="w")
        for name in ("t0", "t1", "t2"):
            self.slot_tree.column(name, anchor="e")
        self.slot_tree.grid(row=0, column=0, sticky="nsew")
        slot_scroll = ttk.Scrollbar(slot_table, orient="vertical",
                                     command=self.slot_tree.yview)
        slot_scroll.grid(row=0, column=1, sticky="ns")
        self.slot_tree.configure(yscrollcommand=slot_scroll.set)
        self.slot_tree.bind("<<TreeviewSelect>>", self._on_slot_select)

        edit_row = tk.Frame(slot_card, bg=BG_PANEL)
        edit_row.grid(row=2, column=0, sticky="ew", pady=(6, 0))
        ttk.Label(edit_row, text="Coat texture", style="Dim.TLabel").pack(side="left")
        self.coat_var = tk.StringVar()
        self.coat_box = ttk.Combobox(edit_row, textvariable=self.coat_var,
                                     state="readonly")
        self.coat_box.pack(side="left", fill="x", expand=True, padx=(8, 0))
        self.coat_box.bind("<<ComboboxSelected>>", self._on_coat_selected)

        edit_row2 = tk.Frame(slot_card, bg=BG_PANEL)
        edit_row2.grid(row=3, column=0, sticky="ew", pady=(4, 0))
        ttk.Label(edit_row2, text="Slot tints", style="Dim.TLabel").pack(side="left")
        self.slot_tint_vars = []
        for _ in range(3):
            var = tk.StringVar(value="0")
            entry = ttk.Entry(edit_row2, textvariable=var, width=4)
            entry.pack(side="left", padx=(5, 0))
            self.slot_tint_vars.append(var)
        ttk.Button(edit_row2, text="Apply tints",
                   command=self._apply_slot_tints).pack(side="left", padx=(10, 0))
        ttk.Button(edit_row2, text="Reset all changes",
                   command=self._reset_changes).pack(side="left", padx=(6, 0))

    def _build_part_card(self, card, part):
        card.columnconfigure(1, weight=1)
        rows = []

        if part == Tag.PART_EYES:

            eye_row = tk.Frame(card, bg=BG_PANEL)
            eye_row.grid(row=1, column=0, columnspan=3, sticky="ew", pady=(2, 0))
            eye_row.columnconfigure(1, weight=1)
            ttk.Label(eye_row, text="Eye colour", style="Dim.TLabel").grid(
                row=0, column=0, sticky="w")
            eye_colour_var = tk.StringVar()
            eye_colour_box = ttk.Combobox(eye_row, textvariable=eye_colour_var,
                                          state="readonly")
            eye_colour_box.grid(row=0, column=1, sticky="ew", padx=(8, 0))
            eye_colour_box.bind("<<ComboboxSelected>>", self._on_eye_colour)
            self.part_cards[part] = {
                "info": None, "tint_vars": [], "preset_var": None,
                "preset_box": None, "extra_row": eye_row,
                "eye_colour_var": eye_colour_var,
                "eye_colour_box": eye_colour_box, "rows": tuple(rows),
            }
            return

        info = ttk.Label(card, text="", style="Dim.TLabel")
        info.grid(row=0, column=0, columnspan=3, sticky="w", pady=(0, 4))

        tint_row = tk.Frame(card, bg=BG_PANEL)
        tint_row.grid(row=1, column=0, columnspan=3, sticky="ew", pady=(2, 0))
        rows.append(tint_row)
        ttk.Label(tint_row, text="Tint", style="Dim.TLabel").grid(
            row=0, column=0, sticky="w")
        tint_vars = []
        for index, name in enumerate(("tint0", "tint1", "tint2")):
            ttk.Label(tint_row, text=name, style="Dim.TLabel").grid(
                row=0, column=1 + index * 2, sticky="w", padx=(6 if index == 0 else 8, 0))
            var = tk.StringVar(value="0")
            entry = ttk.Entry(tint_row, textvariable=var, width=4)
            entry.grid(row=0, column=2 + index * 2, sticky="w", padx=(3, 0))
            entry.bind("<Return>", lambda e, p=part: self._apply_part_tints(p))
            entry.bind("<FocusOut>", lambda e, p=part: self._apply_part_tints(p))
            tint_vars.append(var)

        preset_var = tk.StringVar(value="- pick a horse -")
        preset_box = ttk.Combobox(tint_row, textvariable=preset_var,
                                  state="readonly")
        preset_box.grid(row=0, column=8, sticky="ew", padx=(8, 0))
        preset_box.bind("<<ComboboxSelected>>",
                        lambda e, p=part: self._on_preset_selected(p))
        ttk.Label(tint_row, text="Copy tint from", style="Dim.TLabel").grid(
            row=0, column=7, sticky="e", padx=(14, 0))
        tint_row.columnconfigure(8, weight=1)

        eye_row = None
        eye_colour_var = tk.StringVar()
        eye_colour_box = None

        self.part_cards[part] = {
            "info": info,
            "tint_vars": tint_vars, "preset_var": preset_var,
            "preset_box": preset_box, "extra_row": eye_row,
            "eye_colour_var": eye_colour_var, "eye_colour_box": eye_colour_box,
            "rows": tuple(rows),
        }


    def _build_save_tab(self, notebook):
        save_tab = tk.Frame(notebook, bg=BG_PANEL)
        notebook.add(save_tab, text="  Save  ")
        save_tab.columnconfigure(0, weight=1)

        save_card = self._card(save_tab, "OUTPUT", row=0, sticky="ew",
                               padx=14, pady=(14, 0))
        save_card.columnconfigure(1, weight=1)
        ttk.Label(save_card, text="Vanilla (read-only)", style="Dim.TLabel").grid(
            row=0, column=0, sticky="w", pady=4)
        self.save_vanilla = tk.Label(save_card, text="", bg=BG_PANEL, fg=FG,
                                     font=("Consolas", 9), anchor="w")
        self.save_vanilla.grid(row=0, column=1, sticky="ew", pady=4)
        ttk.Label(save_card, text="Modded (output)", style="Dim.TLabel").grid(
            row=1, column=0, sticky="w", pady=4)
        self.save_modded = tk.Label(save_card, text="", bg=BG_PANEL, fg=FG,
                                    font=("Consolas", 9), anchor="w")
        self.save_modded.grid(row=1, column=1, sticky="ew", pady=4)

        note = ("The vanilla folder is opened read-only and is never written to.\n"
                "Saving writes an updated copy of the selected .ymt into the Modded\n"
                "folder, keeping the original file name and the exact byte layout\n"
                "(BOM, CRLF, indentation) of the source file.")
        ttk.Label(save_card, text=note, style="Dim.TLabel",
                  justify="left").grid(row=2, column=0, columnspan=2,
                                       sticky="w", pady=(10, 0))

        actions = self._card(save_tab, "ACTIONS", row=1, sticky="ew",
                             padx=14, pady=(12, 0))
        ttk.Button(actions, text="Save horse to Modded",
                   style="Accent.TButton",
                   command=self._save_current).pack(side="left")
        ttk.Button(actions, text="Save every horse to Modded",
                   command=self._save_all).pack(side="left", padx=(8, 0))
        ttk.Button(actions, text="Copy vanilla file unchanged",
                   command=self._copy_unchanged).pack(side="left", padx=(8, 0))
        ttk.Button(actions, text="Open Modded folder",
                   command=self._open_modded).pack(side="left", padx=(8, 0))


    def fit_horse_list(self):
        try:
            font = ttk.Style(self).lookup("Treeview", "font")
            measure = tkfont.Font(root=self, font=font).measure
        except Exception:
            def measure(text):
                return len(text) * 6
        longest = ""
        for record in getattr(self, "records", []):
            if measure(record.display_name) > measure(longest):
                longest = record.display_name
        text_width = measure(longest) + 22
        if hasattr(self, "tree"):
            self.tree.column("#0", width=text_width, minwidth=text_width)

        return text_width + 55 + 20 + 26
    def list_panel_width(self):
        try:
            font = ttk.Style(self).lookup("Treeview", "font")
            measure = tkfont.Font(root=self, font=font).measure
        except Exception:
            def measure(text):
                return len(text) * 6
        longest = ""
        for record in getattr(self, "records", []):
            if measure(record.display_name) > measure(longest):
                longest = record.display_name

        return min(measure(longest) + 30 + 55 + 30, 560)

    def _scrollable_tab(self, notebook, title):
        outer = tk.Frame(notebook, bg=BG_PANEL)
        notebook.add(outer, text=title)
        canvas = tk.Canvas(outer, bg=BG_PANEL, highlightthickness=0, bd=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        inner = tk.Frame(canvas, bg=BG_PANEL)
        window = canvas.create_window((0, 0), window=inner, anchor="nw")
        inner.bind("<Configure>", lambda e: self._sync_scrollbar())
        canvas.bind("<Configure>", lambda e: canvas.itemconfigure(window, width=e.width))
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        for widget in (canvas, inner):
            widget.bind("<MouseWheel>",
                        lambda e: canvas.yview_scroll(int(-e.delta / 120), "units"))
        self.canvas = canvas
        self.scrollbar = scrollbar
        return inner

    def _sync_scrollbar(self):
        canvas = getattr(self, "canvas", None)
        scrollbar = getattr(self, "scrollbar", None)
        if canvas is None or scrollbar is None:
            return
        bbox = canvas.bbox("all")
        if bbox:
            canvas.configure(scrollregion=bbox)
        needed = bool(bbox) and bbox[3] > canvas.winfo_height()
        if needed and not scrollbar.winfo_ismapped():
            scrollbar.pack(side="right", fill="y")
        elif not needed and scrollbar.winfo_ismapped():
            scrollbar.pack_forget()

    def _card(self, master, title, row=0, column=0, **grid_kwargs):
        card = tk.Frame(master, bg=BG_PANEL, highlightthickness=1,
                        highlightbackground=BORDER)
        card.grid(row=row, column=column, **grid_kwargs)
        ttk.Label(card, text=title, style="Head.TLabel").grid(
            row=0, column=0, sticky="w", padx=10, pady=(6, 2))
        inner = tk.Frame(card, bg=BG_PANEL)
        inner.grid(row=1, column=0, sticky="nsew", padx=10, pady=(0, 8))
        card.columnconfigure(0, weight=1)
        card.rowconfigure(1, weight=1)
        return inner

    def _bind_keys(self):
        self.bind("<Control-s>", lambda e: self._save_current())
        self.bind("<Control-f>", lambda e: self.search_entry.focus_set())
        self.bind("<Escape>", lambda e: self._reset_changes())
        self.search_var.trace_add("write", lambda *a: self._refresh_list())

    def _log(self, message):
        self.log.insert("end", message.rstrip() + "\n")
        self.log.see("end")

    def _set_state(self, text):
        self.log_state.configure(text=text)


    def _initial_load(self):
        self.records = load_database(self.vanilla_dir)
        self.record_by_id = {r.filename: r for r in self.records}
        self.coat_catalogue, self.coat_users = build_coat_catalogue(self.records)
        self.coat_values = {}
        self.coat_options = []
        for body_hash, (coat_label, other) in sorted(
                self.coat_catalogue.items(), key=lambda kv: kv[1][0].lower()):
            users = self.coat_users.get(body_hash) or []
            who = users[0] if users else ""
            label = coat_label.split(" (")[0].strip()
            text = "%s   (%s)" % (label, who)
            self.coat_options.append(text)
            self.coat_values[text] = (body_hash, other)
        self.tint_catalogue = build_tint_catalogue(self.records)
        self.scale_catalogue = build_scale_catalogue(self.records)
        self.scale_preset_box.configure(values=self.scale_options())
        breeds = sorted(set(r.breed for r in self.records))
        self.breed_box.configure(values=["All breeds"] + breeds)
        self.vanilla_path_label.configure(text=os.path.abspath(self.vanilla_dir))
        self.save_vanilla.configure(text=os.path.abspath(self.vanilla_dir))
        self.save_modded.configure(text=os.path.abspath(self.modded_dir))
        self._refresh_list()

        width = max(490, self.fit_horse_list())
        self.body.columnconfigure(0, minsize=width)
        screen = self.winfo_screenwidth()
        self.minsize(min(width + 560, screen - 80), 660)
        children = self.tree.get_children()
        if children:
            self.tree.selection_set(children[0])
            self.tree.focus(children[0])
            self._on_select()
        self._log("Loaded %d horse models from %s" %
                  (len(self.records), os.path.abspath(self.vanilla_dir)))
        self._set_state("%d horses" % len(self.records))

    def _refresh_list(self):
        if not hasattr(self, "tree"):
            return
        needle = self.search_var.get().strip().lower()
        breed = self.breed_var.get()
        self.tree.delete(*self.tree.get_children())
        shown = 0
        for record in self.records:
            if breed and breed != "All breeds" and record.breed != breed:
                continue
            if needle and needle not in record.search_text:
                continue
            modified = ""
            try:
                if os.path.exists(self._output_path(record.filename)):
                    modified = "M"
            except Exception:
                pass
            self.tree.insert("", "end", iid=record.filename,
                             text=record.display_name,
                             values=("%.5f" % record.scale,),
                             tags=("modded",) if modified else ())
            shown += 1
        self.tree.tag_configure("modded", foreground="#7fd18c")
        self._set_state("%d / %d horses" % (shown, len(self.records)))

    def _on_select(self, _event=None):
        selection = self.tree.selection()
        if not selection:
            return
        self.current = self.record_by_id.get(selection[0])
        self._populate_detail()


    def _populate_detail(self):
        record = self.current
        if record is None:
            return
        self.dirty = False
        self.detail_title.configure(text=record.display_name)
        self.detail_sub.configure(text=record.filename)
        self.scale_var.set("%.8f" % record.scale)
        self._refresh_part_cards()
        self._refresh_slot_tree()
        self._update_state_label()

    def _slot_for_part(self, part):
        if self.current is None:
            return None
        for slot in self.current.slots:
            if slot.part == part:
                return slot
        return None

    def _refresh_part_cards(self):
        for part, widgets in self.part_cards.items():
            slot = self._slot_for_part(part)


            if part == Tag.PART_EYES:
                eyes = widgets
                eyes["eye_colour_box"].configure(
                    values=[label for _t, label in EYE_TEXTURES])
                eyes["eye_colour_box"].configure(state="readonly")
                if self.current is not None:
                    eyes["eye_colour_var"].set(
                        EYE_TEXTURES[self.current.eye_colour][1])
                continue

            if slot is None:
                widgets["preset_box"].configure(state="disabled")
                for var in widgets["tint_vars"]:
                    var.set("")
                widgets["preset_var"].set("- pick a horse -")
                for row in widgets["rows"]:
                    row.grid_remove()
                widgets["info"].configure(text="")
                widgets["info"].grid_remove()
                continue

            for row in widgets["rows"]:
                row.grid()
            widgets["preset_box"].configure(state="readonly")
            widgets["info"].grid_remove()
            for var, value in zip(widgets["tint_vars"], slot.tints):
                var.set(str(value))


            presets = []
            for (ppart, drawable), table in self.tint_catalogue.items():
                if ppart != part:
                    continue
                for tints, names in table.items():
                    presets.append("%s  (%s)" % ("/".join(str(t) for t in tints),
                                                names[0]))
            widgets["preset_box"].configure(values=presets)


        body_slot = self._slot_for_part(Tag.PART_BODY)
        if body_slot is not None:
            self.coat_box.configure(state="readonly", values=self.coat_options)
            current = ""
            for text, (body_hash, other) in self.coat_values.items():
                if body_hash == body_slot.albedo_int:
                    current = text
                    break
            self.coat_var.set(current)
        else:
            self.coat_box.configure(state="disabled")
            self.coat_var.set("")

    def _refresh_slot_tree(self):
        self.slot_tree.delete(*self.slot_tree.get_children())
        if self.current is None:
            return
        for slot in self.current.slots:
            asset = TEXTURE_NAMES.get(slot.albedo_int, slot.albedo)
            self.slot_tree.insert(
                "", "end", iid=str(slot.index),
                values=(slot.part, slot.drawable, asset,
                        slot.tints[0], slot.tints[1], slot.tints[2]))

    def _on_slot_select(self, _event=None):
        selection = self.slot_tree.selection()
        if not selection or self.current is None:
            return
        try:
            slot = self.current.slots[int(selection[0])]
        except (ValueError, IndexError):
            return
        for var, value in zip(self.slot_tint_vars, slot.tints):
            var.set(str(value))


    def _apply_scale(self):
        if self.current is None:
            return
        try:
            value = float(self.scale_var.get())
        except (TypeError, ValueError):
            self.scale_var.set("%.8f" % self.current.vanilla_scale)
            return
        value = max(0.1, min(10.0, value))
        self.scale_var.set("%.8f" % value)
        if abs(value - self.current.scale) < 1e-9:
            return
        self.current.scale = value
        self.dirty = True
        self._refresh_list()
        self._update_state_label()

    def _reset_scale(self):
        if self.current is None:
            return
        value = self.current.vanilla_scale
        self.scale_var.set("%.8f" % value)
        self.current.scale = value
        self.dirty = False
        self._refresh_list()
        self._update_state_label()

    def _apply_part_tints(self, part):
        slot = self._slot_for_part(part)
        if slot is None:
            return
        widgets = self.part_cards[part]
        values = []
        for var in widgets["tint_vars"]:
            try:
                values.append(max(0, min(255, int(var.get().strip() or 0))))
            except ValueError:
                return
        if tuple(values) != slot.tints:
            slot.tints = tuple(values)
            self.dirty = True
            self._refresh_slot_tree()
            self._update_state_label()

    def _on_coat_selected(self, _event=None):
        if self.current is None:
            return
        choice = self.coat_var.get()
        entry = getattr(self, "coat_values", {}).get(choice)
        if entry is None:
            return
        body_hash, other = entry
        body_slot = self._slot_for_part(Tag.PART_BODY)
        head_slot = self._slot_for_part(Tag.PART_HEAD)
        if body_slot is not None:
            body_slot.albedo = "0x%08X" % body_hash
        if head_slot is not None and other:
            head_slot.albedo = "0x%08X" % other
        self.dirty = True
        self._refresh_part_cards()
        self._refresh_slot_tree()
        self._update_state_label()
        self._log("Applied coat '%s' to %s" % (choice.split("(")[0].strip(),
                                               self.current.filename))

    def _on_preset_selected(self, part):
        slot = self._slot_for_part(part)
        if slot is None:
            return
        widgets = self.part_cards[part]
        choice = widgets["preset_var"].get()
        for (ppart, drawable), table in self.tint_catalogue.items():
            if ppart != part:
                continue
            for tints, names in table.items():
                text = "%s  (%s)" % ("/".join(str(t) for t in tints), names[0])
                if text != choice:
                    continue
                slot.tints = tints
                for var, value in zip(widgets["tint_vars"], tints):
                    var.set(str(value))
                self.dirty = True
                self._refresh_slot_tree()
                self._update_state_label()
                return

    def _apply_slot_tints(self):
        selection = self.slot_tree.selection()
        if not selection or self.current is None:
            return
        try:
            slot = self.current.slots[int(selection[0])]
        except (ValueError, IndexError):
            return
        values = []
        for var in self.slot_tint_vars:
            try:
                values.append(max(0, min(255, int(var.get().strip() or 0))))
            except ValueError:
                messagebox.showwarning("Invalid value",
                                       "Tints must be whole numbers between 0 and 255.")
                return
        slot.tints = tuple(values)
        self.dirty = True
        self._refresh_slot_tree()
        self._refresh_part_cards()
        self._update_state_label()

    def _on_eye_colour(self, _event=None):
        if self.current is None:
            return
        widgets = self.part_cards[Tag.PART_EYES]
        label = widgets["eye_colour_var"].get()
        index = 0
        for position, (_texture, name) in enumerate(EYE_TEXTURES):
            if name == label:
                index = position
                break
        self.current.eye_colour = index
        texture = EYE_TEXTURES[index][0]
        slot = self._slot_for_part(Tag.PART_EYES)
        if slot is not None:
            slot.albedo = "0x%08X" % texture
        self.dirty = True
        self._refresh_part_cards()
        self._refresh_slot_tree()
        self._update_state_label()
        self._log("Eye colour of %s set to %s"
                  % (self.current.filename, EYE_TEXTURES[index][1]))

    def scale_options(self):
        options = []
        for value in sorted(self.scale_catalogue):
            labels = self.scale_catalogue[value]
            who = ", ".join(labels)
            if len(labels) > 5:
                who = ", ".join(labels[:5]) + ", +%d more" % (len(labels) - 5)
            options.append("%.5f   (%s)" % (value, who))
        return options

    def _on_scale_preset(self, _event=None):
        if self.current is None:
            return
        choice = self.scale_preset_var.get()
        for value in sorted(self.scale_catalogue):
            labels = self.scale_catalogue[value]
            who = ", ".join(labels)
            if len(labels) > 5:
                who = ", ".join(labels[:5]) + ", +%d more" % (len(labels) - 5)
            if "%.5f   (%s)" % (value, who) != choice:
                continue
            self.scale_var.set("%.8f" % value)
            self._apply_scale()
            self.scale_preset_var.set("- pick a horse -")
            return

    def _reset_changes(self):
        if self.current is None:
            return
        try:
            self.current.document = YmtDocument(self.current.document.path)
        except Exception as exc:
            messagebox.showerror("Error", str(exc))
            return
        record = HorseRecord(self.current.document)
        self.record_by_id[record.filename] = record
        self.current = record
        self._populate_detail()
        self._refresh_list()
        self._log("Reverted %s to the vanilla values." % record.filename)


    def _output_path(self, filename):
        target = os.path.abspath(os.path.join(self.modded_dir, filename))
        root = os.path.abspath(self.modded_dir)
        if os.path.commonpath([root, target]) != root:
            raise RuntimeError("Refusing to write outside the Modded folder.")
        if os.path.abspath(target).startswith(os.path.abspath(self.vanilla_dir) + os.sep):
            raise RuntimeError("Refusing to write inside the Vanilla folder.")
        return target

    def _save_current(self):
        if self.current is None:
            messagebox.showinfo(APP_NAME, "Select a horse first.")
            return
        record = self.current
        try:
            target = self._output_path(record.filename)
            os.makedirs(self.modded_dir, exist_ok=True)
        except Exception as exc:
            messagebox.showerror("Error", str(exc))
            return

        scale = None
        if abs(record.scale - record.vanilla_scale) > 1e-9:
            scale = record.scale
        offset = None

        tint_changes = {}
        texture_changes = {}
        palette_changes = {}
        for slot in record.slots:
            original_tints = record.vanilla_tints.get(slot.drawable, slot.tints)
            if slot.tints != original_tints:
                tint_changes[slot.drawable] = slot.tints
            original_texture = record.vanilla_textures.get(slot.drawable, slot.albedo)
            if slot.albedo != original_texture:
                texture_changes[slot.drawable] = slot.albedo
            original_palette = record.vanilla_palettes.get(slot.drawable, slot.palette)
            if slot.palette != original_palette:
                palette_changes[slot.drawable] = slot.palette

        try:
            content = record.document.render(
                scale=scale, tint_changes=tint_changes,
                texture_changes=texture_changes, palette_changes=palette_changes,
                offset=offset, redness=None,
                scale_all_lods=True)
            with open(target, "wb") as handle:
                handle.write(record.document.write_bytes(content))
        except Exception as exc:
            messagebox.showerror("Error", "Could not save the file:\n%s" % exc)
            return
        self.dirty = False
        self._refresh_list()
        self._update_state_label()
        self._log("Saved %s -> %s" % (record.filename, target))

    def _save_all(self):
        """Save every horse to Modded.

        Horses changed while the tool is open are saved from their current
        in-memory state. Horses without changes are copied from Vanilla
        unchanged.
        """
        try:
            os.makedirs(self.modded_dir, exist_ok=True)
        except OSError as exc:
            messagebox.showerror("Error", str(exc))
            return

        saved = 0
        modified = 0
        copied = 0

        for record in self.records:
            try:
                target = self._output_path(record.filename)

                scale = None
                if abs(record.scale - record.vanilla_scale) > 1e-9:
                    scale = record.scale

                tint_changes = {}
                texture_changes = {}
                palette_changes = {}

                for slot in record.slots:
                    original_tints = record.vanilla_tints.get(
                        slot.drawable, slot.tints)
                    if slot.tints != original_tints:
                        tint_changes[slot.drawable] = slot.tints

                    original_texture = record.vanilla_textures.get(
                        slot.drawable, slot.albedo)
                    if slot.albedo != original_texture:
                        texture_changes[slot.drawable] = slot.albedo

                    original_palette = record.vanilla_palettes.get(
                        slot.drawable, slot.palette)
                    if slot.palette != original_palette:
                        palette_changes[slot.drawable] = slot.palette

                has_changes = (
                    scale is not None
                    or bool(tint_changes)
                    or bool(texture_changes)
                    or bool(palette_changes)
                )

                if has_changes:
                    content = record.document.render(
                        scale=scale,
                        tint_changes=tint_changes,
                        texture_changes=texture_changes,
                        palette_changes=palette_changes,
                        offset=None,
                        redness=None,
                        scale_all_lods=True,
                    )
                    with open(target, "wb") as handle:
                        handle.write(record.document.write_bytes(content))
                    modified += 1
                    self._log("Saved modified %s -> %s" %
                              (record.filename, target))
                else:
                    with open(record.document.path, "rb") as src:
                        with open(target, "wb") as dst:
                            shutil.copyfileobj(src, dst)
                    copied += 1

                saved += 1

            except Exception as exc:
                self._log("Could not save %s: %s" %
                          (record.filename, exc))

        self.dirty = False
        self._refresh_list()
        self._update_state_label()
        self._log(
            "Saved %d horses into %s (%d modified, %d unchanged vanilla copies)"
            % (saved, self.modded_dir, modified, copied)
        )

    def _copy_unchanged(self):
        if self.current is None:
            return
        try:
            target = self._output_path(self.current.filename)
            os.makedirs(self.modded_dir, exist_ok=True)
            with open(self.current.document.path, "rb") as src:
                with open(target, "wb") as dst:
                    shutil.copyfileobj(src, dst)
        except Exception as exc:
            messagebox.showerror("Error", str(exc))
            return
        self._refresh_list()
        self._log("Copied %s unchanged into Modded." % self.current.filename)

    def _open_modded(self):
        path = os.path.abspath(self.modded_dir)
        try:
            os.makedirs(path, exist_ok=True)
        except OSError:
            pass
        try:
            if sys.platform.startswith("win"):
                os.startfile(path)
            elif sys.platform == "darwin":
                subprocess.Popen(["open", path])
            else:
                subprocess.Popen(["xdg-open", path])
        except Exception as exc:
            messagebox.showerror("Error", str(exc))

    def _update_state_label(self):
        if self.current is None:
            self._set_state("")
            return
        state = "modified" if self.dirty else "unchanged"
        self._set_state("%s - %s" % (self.current.filename, state))



def main():
    vanilla = VANILLA_DIR
    modded = MODDED_DIR
    args = sys.argv[1:]
    for index, argument in enumerate(args):
        if argument.lower() == "--vanilla" and index + 1 < len(args):
            vanilla = args[index + 1]
        if argument.lower() == "--modded" and index + 1 < len(args):
            modded = args[index + 1]
    if not os.path.isdir(vanilla):
        root = filedialog.askdirectory(title="Select the Vanilla horse folder")
        if root:
            vanilla = root
    app = HorseManagerApp(vanilla, modded)
    app.mainloop()


if __name__ == "__main__":
    main()
