Search.setIndex({"docnames": ["0_introduction", "10_occlusion_culling", "11_clustered_shading", "1_architecture_overview", "2_render_graph", "3_basic_renderer", "4_debug_renderer", "5_mesh_shader_renderer", "6_reduce", "7_blelloch_scan", "8_radix_sort", "9_bucket_sort", "index"], "filenames": ["0_introduction.rst", "10_occlusion_culling.rst", "11_clustered_shading.rst", "1_architecture_overview.rst", "2_render_graph.rst", "3_basic_renderer.rst", "4_debug_renderer.rst", "5_mesh_shader_renderer.rst", "6_reduce.rst", "7_blelloch_scan.rst", "8_radix_sort.rst", "9_bucket_sort.rst", "index.rst"], "titles": ["Introduction", "Occlusion culling", "Clustered shading", "Architecture overview", "Render-graph", "Basic renderer", "Debug renderer", "Mesh-shader renderer", "Reduce", "Blelloch scan", "GPU radix-sort", "Bucket sort", "Table of contents"], "terms": {"lorem": [0, 1, 2, 3, 5, 6, 7, 8, 9, 11], "ipsum": [0, 1, 2, 3, 5, 6, 7, 8, 9, 11], "The": [4, 10], "sequenc": 4, "command": 4, "need": [4, 10], "frame": 4, "can": [4, 10], "grow": 4, "s": [4, 10], "logic": 4, "becom": 4, "more": [4, 10], "complex": 4, "These": 4, "involv": 4, "object": 4, "e": [4, 10], "g": [4, 10], "buffer": [4, 10], "imag": 4, "whose": [4, 10], "access": [4, 10], "must": 4, "correctli": 4, "synchron": 4, "avoid": 4, "undefin": 4, "behavior": 4, "read": [4, 10], "happen": 4, "concurr": [4, 10], "write": [4, 10], "wai": [4, 10], "ensur": 4, "memori": [4, 10], "us": [4, 10], "If": [4, 10], "you": 4, "alreadi": 4, "know": [4, 10], "what": 4, "skip": 4, "thi": [4, 10], "section": 4, "assum": 4, "we": [4, 10], "have": [4, 10], "simpl": 4, "made": 4, "comput": [4, 10], "task": 4, "c1": 4, "an": [4, 10], "i1": 4, "anoth": [4, 10], "c2": 4, "ha": [4, 10], "from": [4, 10], "same": [4, 10], "And": 4, "consid": [4, 10], "follow": [4, 10], "pseudo": 4, "code": 4, "dispatch": [4, 10], "bind": 4, "after": [4, 10], "1": [4, 10], "onli": [4, 10], "oper": [4, 10], "start": [4, 10], "therefor": 4, "assur": 4, "all": [4, 10], "complet": 4, "so": [4, 10], "offer": 4, "ar": [4, 10], "place": [4, 10], "between": 4, "two": 4, "interest": 4, "i": [4, 10], "which": [4, 10], "defin": 4, "set": 4, "sourc": 4, "c1_op": 4, "destin": [4, 10], "c2_op": 4, "when": [4, 10], "along": [4, 10], "confid": 4, "befor": [4, 10], "let": [4, 10], "up": [4, 10], "accomplish": 4, "our": 4, "problem": 4, "place_pipeline_memory_barri": 4, "valid": 4, "doesn": 4, "t": [4, 10], "repres": 4, "As": [4, 10], "mai": 4, "want": [4, 10], "mani": [4, 10], "depend": 4, "In": [4, 10], "case": [4, 10], "would": [4, 10], "obviou": 4, "over": [4, 10], "issu": [4, 10], "becaus": 4, "now": [4, 10], "wait": 4, "ani": [4, 10], "while": [4, 10], "d1": 4, "second": 4, "i2": 4, "block": [4, 10], "forc": 4, "d2": 4, "d3": 4, "d4": 4, "act": 4, "here": 4, "For": [4, 10], "purpos": 4, "possibl": 4, "narrow": 4, "specifi": 4, "wherev": 4, "long": 4, "re": [4, 12], "sure": 4, "actual": 4, "lot": 4, "exampl": [4, 10], "exact": 4, "stage": 4, "belong": 4, "type": 4, "moreov": [4, 10], "layout": 4, "transit": 4, "some": 4, "better": [4, 10], "certain": [4, 10], "handl": 4, "each": [4, 10], "void": 4, "vkcmdpipelinebarri": 4, "vkcommandbuff": 4, "commandbuff": 4, "vkpipelinestageflag": 4, "srcstagemask": 4, "dststagemask": 4, "vkdependencyflag": 4, "dependencyflag": 4, "uint32_t": 4, "memorybarriercount": 4, "const": 4, "vkmemorybarri": 4, "pmemorybarri": 4, "buffermemorybarriercount": 4, "vkbuffermemorybarri": 4, "pbuffermemorybarri": 4, "imagememorybarriercount": 4, "vkimagememorybarri": 4, "pimagememorybarri": 4, "struct": 4, "instanti": 4, "typedef": 4, "vkstructuretyp": 4, "stype": 4, "pnext": 4, "vkaccessflag": 4, "srcaccessmask": 4, "dstaccessmask": 4, "vkimagelayout": 4, "oldlayout": 4, "newlayout": 4, "srcqueuefamilyindex": 4, "dstqueuefamilyindex": 4, "vkimag": 4, "vkimagesubresourcerang": 4, "subresourcerang": 4, "vkbuffer": 4, "vkdevices": 4, "offset": [4, 12], "size": [4, 10], "To": 4, "keep": [4, 10], "identifi": 4, "usag": 4, "approach": [4, 10], "everi": [4, 10], "field": 4, "take": [4, 10], "advantag": 4, "low": 4, "level": 4, "provid": [4, 10], "api": 4, "few": 4, "number": [4, 12], "just": 4, "almost": 4, "certainli": 4, "It": [4, 10], "achiev": 4, "best": 4, "perform": [4, 10], "choos": 4, "realli": 4, "hard": 4, "maintain": 4, "structur": 4, "chang": 4, "insert": 4, "move": 4, "delet": 4, "could": [4, 10], "probabl": 4, "deal": 4, "tediou": 4, "error": 4, "architectur": [4, 12], "come": [4, 10], "aid": 4, "permit": 4, "fly": 4, "try": 4, "get": [4, 10], "rid": 4, "detail": [4, 10], "vren": 10, "implement": 10, "algorithm": 10, "veri": 10, "well": 10, "larg": 10, "constrain": 10, "32": 10, "bit": 10, "integ": 10, "kei": 10, "easi": 10, "see": 10, "practic": 10, "sequenti": 10, "arrai": 10, "10": 10, "base": 10, "25": 10, "39": 10, "92": 10, "5": 10, "68": 10, "23": 10, "21": 10, "iter": 10, "occurr": 10, "least": 10, "signific": 10, "ls": 10, "digit": 10, "0": 10, "2": 10, "4": 10, "3": 10, "6": 10, "8": 10, "7": 10, "9": 10, "obtain": 10, "column": 10, "run": 10, "again": 10, "its": 10, "posit": 10, "note": 10, "than": 10, "one": 10, "fall": 10, "both": 10, "should": 10, "reason": 10, "counter": 10, "increment": 10, "said": 10, "look": [10, 12], "first": 10, "back": 10, "next": 10, "until": 10, "most": 10, "ms": 10, "100": 10, "sai": 10, "1st": 10, "gener": 10, "friendli": 10, "power": 10, "16": 10, "fairli": 10, "easili": 10, "abov": 10, "saw": 10, "split": 10, "right": 10, "left": 10, "turn": 10, "step": 10, "includ": 10, "n": 10, "effici": 10, "trivial": 10, "conveni": 10, "visit": 10, "workgroup": 10, "ceil": 10, "dumb": 10, "creat": 10, "u32": 10, "atomicincr": 10, "time": 10, "found": 10, "1m": 10, "obvious": 10, "ineffici": 10, "huge": 10, "amount": 10, "invoc": 10, "atom": 10, "16x4": 10, "slot": 10, "do": [10, 12], "restrict": 10, "within": 10, "sub": 10, "origin": 10, "share": 10, "Then": 10, "through": 10, "reduct": 10, "lead": 10, "addit": 10, "fine": 10, "even": 10, "vulkan": [10, 12], "introduc": 10, "extens": 10, "altern": 10, "intra": 10, "instead": 10, "hold": 10, "hexadecim": 10, "There": 10, "evid": 10, "ve": 10, "examin": 10, "thread": 10, "also": 10, "differ": 10, "item": 10, "valu": 10, "out": 10, "previou": 10, "go": 10, "those": 10, "talk": 10, "other": 10, "reserv": 10, "uniqu": 10, "output": 10, "solut": 10, "But": 10, "reject": 10, "similar": 10, "procedur": 10, "maxim": 10, "had": 10, "store": 10, "refer": 10, "sum": 10, "togeth": 10, "index": 10, "tell": 10, "where": 10, "accord": 10, "14": 10, "variabl": 10, "subgroup": 10, "arithmet": 10, "final": 10, "correct": 10, "describ": 10, "earlier": 10, "understand": 10, "fact": 10, "requir": 10, "temporari": 10, "big": 10, "input": 10, "ll": 10, "swap": 10, "were": 10, "per": 10, "64": 10, "introduct": [10, 12], "harada": 10, "l": 10, "link": 10, "broken": 10, "fast": 10, "j": 10, "kruger": 10, "c": 10, "silva": 10, "nabla": 10, "radixsort": 10, "udac": 10, "program": 10, "tutori": 10, "overview": 12, "render": 12, "graph": 12, "quick": 12, "pipelin": 12, "barrier": 12, "how": 12, "thei": 12, "conclus": 12, "basic": 12, "debug": 12, "mesh": 12, "shader": 12, "reduc": 12, "blelloch": 12, "scan": 12, "gpu": 12, "radix": 12, "sort": 12, "work": 12, "parallel": 12, "count": 12, "symbol": 12, "exclus": 12, "find": 12, "global": 12, "local": 12, "order": 12, "element": 12, "repeat": 12, "resourc": 12, "bucket": 12, "occlus": 12, "cull": 12, "cluster": 12, "shade": 12}, "objects": {}, "objtypes": {}, "objnames": {}, "titleterms": {"introduct": 0, "occlus": 1, "cull": 1, "cluster": 2, "shade": 2, "architectur": 3, "overview": [3, 4], "render": [4, 5, 6, 7], "graph": 4, "quick": 4, "pipelin": 4, "barrier": 4, "how": [4, 10], "do": 4, "thei": 4, "look": 4, "vulkan": 4, "conclus": 4, "basic": 5, "debug": 6, "mesh": 7, "shader": 7, "reduc": 8, "blelloch": 9, "scan": [9, 10], "gpu": 10, "radix": 10, "sort": [10, 11], "work": 10, "parallel": 10, "count": 10, "number": 10, "symbol": 10, "exclus": 10, "find": 10, "global": 10, "local": 10, "offset": 10, "re": 10, "order": 10, "element": 10, "repeat": 10, "resourc": 10, "bucket": 11, "tabl": 12, "content": 12}, "envversion": {"sphinx.domains.c": 2, "sphinx.domains.changeset": 1, "sphinx.domains.citation": 1, "sphinx.domains.cpp": 6, "sphinx.domains.index": 1, "sphinx.domains.javascript": 2, "sphinx.domains.math": 2, "sphinx.domains.python": 3, "sphinx.domains.rst": 2, "sphinx.domains.std": 2, "sphinx": 56}})