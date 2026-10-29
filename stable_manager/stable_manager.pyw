#!/usr/bin/env python3
import tkinter as tk
from tkinter import ttk, messagebox, Toplevel
import os, sys, re
from collections import OrderedDict

# ─── Stable horse definitions ──────────────────────────────────────────

STABLE_HORSES = {
    "1_": {
        "name": "Valentine - Amos Levi Blacksmith and Farrier",
        "horses": {
            "0x05C70C99": ("A_C_Horse_Morgan_Palomino", "Morgan Palomino", 15, ""),
            "0x0348B323": ("A_C_Horse_AmericanStandardbred_PalominoDapple", "American Standardbred Palomino Dapple", 150, ""),
            "0xCF246898": ("A_C_Horse_HungarianHalfbred_DarkDappleGrey", "Hungarian Halfbred Dapple Dark Grey", 150, ""),
            "0x28F9976A": ("A_C_Horse_DutchWarmblood_ChocolateRoan", "Dutch Warmblood Chocolate Roan", 450, ""),
            "0x35A71C98": ("A_C_Horse_Thoroughbred_ReverseDappleBlack", "Thoroughbred Reverse Dapple Black", 0, "Special/Ultimate Edition"),
            "0x8739A629": ("A_C_Horse_Ardennes_IronGreyRoan", "Ardennes Iron Grey Roan", 0, "Pre-Order Bonus"),
        }
    },
    "2_": {
        "name": "Big Valley - Strawberry Stables",
        "horses": {
            "0xE57FC660": ("A_C_Horse_Andalusian_DarkBay", "Andalusian Dark Bay", 140, ""),
            "0x33598622": ("A_C_Horse_DutchWarmblood_SealBrown", "Dutch Warmblood Seal Brown", 150, ""),
            "0xE0A34BD3": ("A_C_Horse_Thoroughbred_Brindle", "Thoroughbred Brindle", 450, ""),
        }
    },
    "3_": {
        "name": "Scarlett Meadows - Scarlett Horse Shop",
        "horses": {
            "0xA3C3F4C6": ("A_C_Horse_Ardennes_BayRoan", "Ardennes Bay Roan", 140, ""),
            "0x5EF3CBDA": ("A_C_Horse_DutchWarmblood_SootyBuckskin", "Dutch Warmblood Sooty Buckskin", 150, ""),
            "0xB0004639": ("A_C_Horse_MissouriFoxTrotter_AmberChampagne", "Missouri Fox Trotter Amber Champagne", 950, ""),
        }
    },
    "4_": {
        "name": "Saint Denis - Theodore Eckhart Stables",
        "horses": {
            "0x0660E640": ("A_C_Horse_Nokota_ReverseDappleRoan", "Nokota Reverse Dapple Roan", 450, ""),
            "0x6572D46D": ("A_C_Horse_Turkoman_Gold", "Turkoman Gold", 950, ""),
            "0x88D6A59E": ("A_C_Horse_Arabian_Black", "Arabian Black", 1050, ""),
        }
    },
    "5_": {
        "name": "Roanoke Ridge - Livery and Feed Stable",
        "horses": {
            "0x2C80A080": ("A_C_Horse_Andalusian_RoseGrey", "Andalusian Rose Grey", 440, ""),
            "0xDA23037A": ("A_C_Horse_Ardennes_StrawberryRoan", "Ardennes Strawberry Roan", 450, ""),
            "0xBC030D85": ("A_C_Horse_Appaloosa_Leopard", "Appaloosa Leopard", 430, ""),
        }
    },
    "6_": {
        "name": "Blackwater - B. R. Shelton Livery & Stable",
        "horses": {
            "0xBB31267C": ("A_C_Horse_MissouriFoxTrotter_SilverDapplePinto", "Missouri Fox Trotter Silver Dapple Pinto", 950, ""),
            "0x4394FBA4": ("A_C_Horse_Turkoman_DarkBay", "Turkoman Dark Bay", 925, ""),
            "0xE7F3880C": ("A_C_Horse_Arabian_RoseGreyBay", "Arabian Rose Grey Bay", 1250, ""),
        }
    },
    "7_": {
        "name": "A. Aitken - Tumbleweed - A. Aitken Livery & Stable",
        "horses": {
            "0x9C978CB3": ("A_C_Horse_TennesseeWalker_FlaxenRoan", "Tennessee Walker Flaxen Roan", 150, ""),
            "0xE4AD6760": ("A_C_Horse_AmericanStandardbred_SilverTailBuckskin", "American Standardbred Silver Tail Buckskin", 400, ""),
            "0xE52CB9B2": ("A_C_Horse_AmericanPaint_Overo", "American Paint Overo", 425, ""),
            "0xC2A67972": ("A_C_Horse_Appaloosa_BrownLeopard", "Appaloosa Brown Leopard", 450, ""),
            "0xA06225BC": ("A_C_Horse_Turkoman_Silver", "Turkoman Silver", 950, ""),
        }
    },
}

HORSE_HASHES = {
    "0x05C70C99": "Morgan (Palomino)",
    "0x0348B323": "American Standardbred (Palomino Dapple)",
    "0xCF246898": "Hungarian Half-bred (Dark Dapple Grey)",
    "0x28F9976A": "Dutch Warmblood (Chocolate Roan)",
    "0x35A71C98": "Thoroughbred (Reverse Dapple Black)",
    "0x8739A629": "Ardennes (Iron Grey Roan)",
    "0xE57FC660": "Andalusian (Dark Bay)",
    "0x33598622": "Dutch Warmblood (Seal Brown)",
    "0xE0A34BD3": "Thoroughbred (Brindle)",
    "0xA3C3F4C6": "Ardennes (Bay Roan)",
    "0x5EF3CBDA": "Dutch Warmblood (Sooty Buckskin)",
    "0xB0004639": "Missouri Fox Trotter (Amber Champagne)",
    "0x0660E640": "Nokota (Reverse Dapple Roan)",
    "0x6572D46D": "Turkoman (Gold)",
    "0x88D6A59E": "Arabian (Black)",
    "0x2C80A080": "Andalusian (Rose Grey)",
    "0xDA23037A": "Ardennes (Strawberry Roan)",
    "0xBC030D85": "Appaloosa (Leopard)",
    "0xBB31267C": "Missouri Fox Trotter (Silver Dapple Pinto)",
    "0x4394FBA4": "Turkoman (Dark Bay)",
    "0xE7F3880C": "Arabian (Rose Grey Bay)",
    "0x9C978CB3": "Tennessee Walker (Flaxen Roan)",
    "0xE4AD6760": "American Standardbred (Silver Tail Buckskin)",
    "0xE52CB9B2": "American Paint (Grey Overo)",
    "0xC2A67972": "Appaloosa (Brown Leopard)",
    "0xA06225BC": "Turkoman (Silver)",
}

HASH_TO_NAME = {}
for h, n in HORSE_HASHES.items():
    HASH_TO_NAME[h.upper()] = n

# ─── YMT Parser ─────────────────────────────────────────────────────────

class YMTNode:
    def __init__(self, name, attributes=None, text=None):
        self.name = name
        self.attributes = OrderedDict(attributes) if attributes else OrderedDict()
        self.children = []
        self.text = text
        self.parent = None

    def add_child(self, node):
        node.parent = self
        self.children.append(node)

    def get_child_by_name(self, name):
        for c in self.children:
            if c.name == name:
                return c
        return None

    def get_children_by_name(self, name):
        return [c for c in self.children if c.name == name]

    def get_attr(self, key, default=None):
        return self.attributes.get(key, default)

    NEWLINE = "\r\n"

    def to_string(self, indent=0):
        pad = "  " * indent
        attrs = ""
        if self.attributes:
            attrs = " " + " ".join(f'{k}="{v}"' for k, v in self.attributes.items())
        nl = self.NEWLINE
        if self.text is not None:
            return f"{pad}<{self.name}{attrs}>{self.text}</{self.name}>{nl}"
        if not self.children:
            return f"{pad}<{self.name}{attrs}/>{nl}"
        lines = [f"{pad}<{self.name}{attrs}>{nl}"]
        for child in self.children:
            if isinstance(child, YMTNode):
                lines.append(child.to_string(indent + 1))
            else:
                lines.append(f"{pad}  {child}{nl}")
        lines.append(f"{pad}</{self.name}>{nl}")
        return "".join(lines)


class YMTDocument:
    def __init__(self):
        self.root = None

    def parse(self, text):
        self.original_text = text
        self.root = self._parse_element(text, 0)[0]

    def _parse_element(self, text, pos):
        tag_start = text.find('<', pos)
        if tag_start == -1:
            return None, pos
        if text[tag_start+1] == '/':
            return None, text.find('>', tag_start) + 1
        tag_end = text.find('>', tag_start)
        if tag_end == -1:
            return None, pos
        tag_content = text[tag_start+1:tag_end]
        if tag_content.endswith('/'):
            clean = tag_content[:-1].strip()
            name_end = self._find_name_end(clean)
            name = clean[:name_end].strip()
            attr_str = clean[name_end:].strip()
            attrs = self._parse_attributes(attr_str)
            node = YMTNode(name, attrs)
            return node, tag_end + 1
        name_end = self._find_name_end(tag_content)
        name = tag_content[:name_end].strip()
        attr_str = tag_content[name_end:].strip()
        attrs = self._parse_attributes(attr_str)
        node = YMTNode(name, attrs)
        pos = tag_end + 1
        while pos < len(text):
            if text[pos] == '<':
                if text[pos+1] == '/':
                    close_end = text.find('>', pos)
                    close_tag = text[pos+2:close_end].strip()
                    if close_tag == name:
                        return node, close_end + 1
                    pos = close_end + 1
                    continue
                else:
                    child, pos = self._parse_element(text, pos)
                    if child is not None:
                        node.add_child(child)
                    continue
            else:
                text_end = text.find('<', pos)
                if text_end == -1:
                    break
                content = text[pos:text_end].strip()
                if content and not node.children:
                    node.text = content
                pos = text_end
        return node, pos

    def _find_name_end(self, s):
        i = 0
        while i < len(s) and not s[i].isspace():
            i += 1
        return i

    def _parse_attributes(self, s):
        attrs = OrderedDict()
        if not s:
            return attrs
        for match in re.finditer(r'(\S+)\s*=\s*"([^"]*)"', s):
            attrs[match.group(1)] = match.group(2)
        return attrs

    def to_string(self):
        if self.root:
            return self.root.to_string(0)
        return ""

    def get_all_items(self):
        items = []
        self._collect_items(self.root, items)
        return items

    def _collect_items(self, node, items):
        if node.name == 'Item' and 'key' in node.attributes:
            items.append(node)
        for child in node.children:
            self._collect_items(child, items)


class CatalogEditor:
    def __init__(self):
        self.doc = None
        self.filepath = ""
        self.horse_items = []

    def load_file(self, filepath):
        with open(filepath, 'r', encoding='utf-8') as f:
            text = f.read()
        self.doc = YMTDocument()
        self.doc.parse(text)
        self.filepath = filepath
        self._find_horses()
        return True

    def save_file(self, filepath=None):
        if filepath is None:
            filepath = self.filepath
        output = self.doc.to_string()
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.write(output)
        return True

    def _find_horses(self):
        self.horse_items = []
        for item in self.doc.get_all_items():
            horse_hash = self._get_node_text(item, 'UNK_MEMBER_0x6C42F444')
            if horse_hash and horse_hash.strip():
                self.horse_items.append((item, item.get_attr('key')))

    def get_horse_hash(self, item_node):
        return self._get_node_text(item_node, 'UNK_MEMBER_0x6C42F444')

    def get_category(self, item_node):
        return self._get_node_text(item_node, 'category')

    def get_item_key(self, item_node):
        return item_node.get_attr('key')

    def remove_item_by_key(self, item_key):
        for item in self.doc.get_all_items():
            if item.get_attr('key') == item_key:
                parent = item.parent
                if parent:
                    parent.children.remove(item)
                    item.parent = None
                    return item
        return None

    def restore_item(self, item_node):
        root = self.doc.root
        for child in root.children:
            if child.name == 'UNK_MEMBER_0x0622EDA3':
                for sub in child.children:
                    if sub.name == 'UNK_MEMBER_0xC53812FF':
                        sub.add_child(item_node)
                        item_node.parent = sub
                        return True
        return False

    def get_price(self, item_node):
        n4f = item_node.get_child_by_name('UNK_MEMBER_0x4F728F50')
        if n4f is None:
            return ""
        for item in n4f.get_children_by_name('Item'):
            c538 = item.get_child_by_name('UNK_MEMBER_0xC53812FF')
            if c538 is None:
                continue
            for pi in c538.get_children_by_name('Item'):
                value_node = pi.get_child_by_name('UNK_MEMBER_0x9EA8B8F4')
                if value_node is not None:
                    return value_node.get_attr('value', '')
                if 'value' in pi.attributes:
                    return pi.get_attr('value', '')
        return ""

    def set_price(self, item_node, price_cents_str):
        n4f = item_node.get_child_by_name('UNK_MEMBER_0x4F728F50')
        if n4f is None:
            return False
        for item in n4f.get_children_by_name('Item'):
            c538 = item.get_child_by_name('UNK_MEMBER_0xC53812FF')
            if c538 is None:
                continue
            for pi in c538.get_children_by_name('Item'):
                value_node = pi.get_child_by_name('UNK_MEMBER_0x9EA8B8F4')
                if value_node is not None:
                    value_node.attributes['value'] = price_cents_str
                    return True
                if 'value' in pi.attributes:
                    pi.attributes['value'] = price_cents_str
                    return True
        return False

    def _get_node_text(self, node, name):
        child = node.get_child_by_name(name)
        if child is None:
            return ""
        if child.text is not None:
            return child.text
        return ""

# ─── GUI Application ────────────────────────────────────────────────────

class StableManagerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Dreamaker'$ RDR2 Stable Manager")
        self.root.state('zoomed')
        self.root.minsize(450, 350)

        self.editor = CatalogEditor()
        self.ymt_path = os.path.join(script_dir, "catalog_sp.ymt")
        self.bak_path = os.path.join(script_dir, "catalog_sp.bak")
        self._backup = {}
        self._var_map = {}
        self._price_vars = {}
        self._price_entries = {}
        self._status_labels = {}

        self._build_ui()
        self._load(self.bak_path)

    def _build_ui(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=3)
        ttk.Label(toolbar, text="Stable Manager", font=("", 12, "bold")).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Save YMT", command=self._save).pack(side=tk.RIGHT, padx=2)

        self.status = ttk.Label(self.root, text="Ready", relief=tk.SUNKEN, anchor=tk.W)
        self.status.pack(side=tk.BOTTOM, fill=tk.X)

        canvas = tk.Canvas(self.root, highlightthickness=0)
        scrollbar = ttk.Scrollbar(self.root, orient=tk.VERTICAL, command=canvas.yview)
        self.scroll_frame = ttk.Frame(canvas)

        self.scroll_frame.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.create_window((0, 0), window=self.scroll_frame, anchor='nw')
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=3)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    def _load(self, path):
        if not os.path.exists(path):
            self.status.config(text=f"{path} not found!")
            return
        try:
            self.editor.load_file(path)
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load YMT:\n{e}")
            return

        for w in self.scroll_frame.winfo_children():
            w.destroy()
        self._var_map.clear()
        self._price_vars.clear()
        self._price_entries.clear()
        self._status_labels.clear()
        self._backup.clear()

        stable_cats = {"0x9C21E432", "0x6B6CCEAC"}
        self._by_hash = {}
        self._by_key = {}
        for item_node, key in self.editor.horse_items:
            cat = self.editor.get_category(item_node)
            if cat in stable_cats:
                hh = self.editor.get_horse_hash(item_node).upper()
                ik = self.editor.get_item_key(item_node)
                self._by_hash[hh] = (ik, item_node)
                self._by_key[ik] = (hh, item_node)

        row = 0
        total_present = 0
        total_count = 0
        for stable_key in sorted(STABLE_HORSES.keys()):
            stable = STABLE_HORSES[stable_key]
            stable_name = stable["name"]
            stable_horses = stable["horses"]

            header = ttk.Label(self.scroll_frame, text=stable_name,
                               font=("", 10, "bold"), foreground="#2a5c8a")
            header.grid(row=row, column=0, columnspan=5, sticky=tk.W, padx=5, pady=(10, 2))
            row += 1

            for hh, (item_name, desc, price_dollars, chapter) in stable_horses.items():
                hh_upper = hh.upper()
                horse_name = HASH_TO_NAME.get(hh_upper, desc)
                in_ymt = hh_upper in self._by_hash
                is_present = in_ymt

                var = tk.BooleanVar(value=is_present)
                self._var_map[(stable_key, hh)] = var

                chk = ttk.Checkbutton(self.scroll_frame, variable=var,
                                      command=lambda sk=stable_key, h=hh: self._on_toggle(sk, h))
                chk.grid(row=row, column=0, sticky=tk.W, padx=(15, 2))

                name_label = ttk.Label(self.scroll_frame, text=horse_name)
                name_label.grid(row=row, column=1, sticky=tk.W, padx=2)

                # Price column (editable)
                pvar = tk.StringVar()
                if is_present and hh_upper in self._by_hash:
                    _, node = self._by_hash[hh_upper]
                    cents = self.editor.get_price(node)
                    if cents:
                        try:
                            pvar.set(str(int(cents) // 100))
                        except ValueError:
                            pvar.set(str(price_dollars))
                    else:
                        pvar.set(str(price_dollars))
                else:
                    pvar.set(str(price_dollars))
                self._price_vars[(stable_key, hh)] = pvar

                ttk.Label(self.scroll_frame, text="$", foreground="gray25").grid(row=row, column=2, sticky=tk.W, padx=(2,0))
                price_entry = ttk.Entry(self.scroll_frame, textvariable=pvar, width=8)
                price_entry.grid(row=row, column=3, sticky=tk.W, padx=(0,2))
                if not is_present:
                    price_entry.config(state='disabled')
                self._price_entries[(stable_key, hh)] = price_entry

                stext = "OK" if is_present else "Removed"
                fg = "green" if is_present else "red"
                status_label = ttk.Label(self.scroll_frame, text=stext, foreground=fg)
                status_label.grid(row=row, column=4, sticky=tk.W, padx=5)
                self._status_labels[(stable_key, hh)] = status_label

                if is_present:
                    total_present += 1
                total_count += 1
                row += 1

        self.status.config(text=f"Loaded: {total_present}/{total_count} horses present")

    def _save(self):
        if not self.editor.filepath:
            self.status.config(text="No file loaded")
            return

        popup = Toplevel(self.root)
        popup.overrideredirect(True)
        popup.configure(bg="#2a2a2a")
        popup.geometry("+{}+{}".format(
            self.root.winfo_x() + self.root.winfo_width() // 2 - 50,
            self.root.winfo_y() + self.root.winfo_height() // 2 - 10))
        popup.transient(self.root)
        popup.grab_set()
        popup.lift()
        tk.Label(popup, text="Salvando...", font=("", 9),
                 bg="#2a2a2a", fg="white").pack(expand=True, fill=tk.BOTH, padx=20, pady=10)
        popup.update()

        try:
            # Save prices for checked horses
            for (stable_key, hh), pvar in self._price_vars.items():
                hh_upper = hh.upper()
                chk_var = self._var_map.get((stable_key, hh))
                if chk_var and not chk_var.get():
                    continue
                if hh_upper in self._by_hash:
                    _, node = self._by_hash[hh_upper]
                    try:
                        dollars = int(pvar.get().strip())
                        cents = str(dollars * 100)
                        if not self.editor.set_price(node, cents):
                            pass
                    except ValueError:
                        pass

            self.editor.save_file(self.ymt_path)
            self._load(self.ymt_path)
        except Exception as e:
            popup.destroy()
            messagebox.showerror("Error", f"Failed to save:\n{e}")
            return

        popup.destroy()

    def _on_toggle(self, stable_key, hh):
        hh_upper = hh.upper()
        var = self._var_map.get((stable_key, hh))
        if var is None:
            return
        if var.get():
            self._restore(hh_upper)
        else:
            self._remove(hh_upper)
        entry = self._price_entries.get((stable_key, hh))
        if entry:
            entry.config(state='normal' if var.get() else 'disabled')
        self._update_row_label(stable_key, hh)

    def _update_row_label(self, stable_key, hh):
        var = self._var_map.get((stable_key, hh))
        if var is None:
            return
        lbl = self._status_labels.get((stable_key, hh))
        if lbl is None:
            return
        is_present = var.get()
        lbl.config(text="OK" if is_present else "Removed",
                   foreground="green" if is_present else "red")

    def _remove(self, hh_upper):
        if hh_upper in self._by_hash:
            ik, node = self._by_hash[hh_upper]
            xml_backup = node.to_string()
            removed = self.editor.remove_item_by_key(ik)
            if removed:
                self._backup[hh_upper] = {'key': ik, 'xml': xml_backup}
                del self._by_hash[hh_upper]
                if ik in self._by_key:
                    del self._by_key[ik]
                name = HASH_TO_NAME.get(hh_upper, hh_upper)
                self.status.config(text=f"Removed: {name}")
            else:
                self.status.config(text=f"Failed to remove {hh_upper}")
                self._set_checkbox(hh_upper, True)
        elif hh_upper in self._backup:
            self.status.config(text="Already removed")
        else:
            self.status.config(text=f"Not found in YMT: {hh_upper}")
            self._set_checkbox(hh_upper, True)

    def _restore(self, hh_upper):
        if hh_upper not in self._backup:
            self.status.config(text=f"No backup for {hh_upper}")
            self._set_checkbox(hh_upper, False)
            return
        data = self._backup[hh_upper]
        xml_str = data['xml']
        try:
            temp_doc = YMTDocument()
            temp_doc.parse(xml_str)
            if temp_doc.root is None:
                raise ValueError("Empty root")
            restored = temp_doc.root
            if self.editor.restore_item(restored):
                ik = restored.get_attr('key')
                self._by_hash[hh_upper] = (ik, restored)
                self._by_key[ik] = (hh_upper, restored)
                del self._backup[hh_upper]
                name = HASH_TO_NAME.get(hh_upper, hh_upper)
                self.status.config(text=f"Restored: {name}")
            else:
                self.status.config(text="Restore failed (container not found)")
                self._set_checkbox(hh_upper, False)
        except Exception as e:
            self.status.config(text=f"Restore error: {e}")
            self._set_checkbox(hh_upper, False)

    def _set_checkbox(self, hh_upper, value):
        for (sk, hh), var in self._var_map.items():
            if hh.upper() == hh_upper:
                var.set(value)
                return

script_dir = os.path.dirname(os.path.abspath(__file__))

def main():
    root = tk.Tk()
    app = StableManagerApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
