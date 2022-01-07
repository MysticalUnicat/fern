# a copy of cjs.bqn from mlochbaum/BQN and edited for fern

args ← •args
"Usage: tool/cf.bqn path/to/mlochbaum/BQN [-i] module"!2≤≠args
path ← ⊑args ⋄ args ↓˜ ↩ 1
# Javascript/JSON formatting

L ← "{"∾"}"∾˜(0<≠)◶⟨"",1↓·∾","⊸∾¨⟩ # Native list/array

Ind ← {∾𝕨‿"["‿𝕩‿"]"}          # Native list/array indexing
Cat ← {∾𝕨‿".concat("‿𝕩‿")"}   # Native list/array concatenation (like ∾)
# Escape the special characters that appear in BQN sources.
Esc←{
  in ← (@+0‿9‿10‿13)∾"'"""    # Null, Tab, LF, CR, and quotes
  out ← "0tnr"                # Whitespace characters changed to letters
  i ← in⊐𝕩
  𝕩 ↩ i ⊏⟜out⌾((i<≠out)⊸/) 𝕩  # Replace
  ∾(i<≠in) /⟜"\"⊸∾¨ 𝕩         # Insert \
}⍟(0<≠)

MkConst ← {∾ "{ .type = fern_BQN_File_Constant_"‿𝕨‿", ."‿𝕨‿" = "‿𝕩‿"}"}

Str ← {"string" MkConst "u8""" ∾ """" ∾˜ Esc 𝕩}
Char ← {"character" MkConst "U" ∾ "'" (∾∾⊣) Esc ⥊ 𝕩}

F ← •Repr                # Native format

FP ← ∞⊸=◶⟨F,"3.14159265358979311599796346854"⟩ # Format positive number
Num ← {"number" MkConst 0⊸≤◶⟨"-"∾FP∘|,FP⟩ 𝕩}  # Format number

glyphs ← •Import path∾"/src/glyphs.bqn"
_getComp ← { (4+useInd) ↑ (𝕗 •Import path∾"/src/c.bqn"){𝔽} }
useInd ← "-i"≡⊑args ⋄ args↓˜↩useInd

ConstImport ← {< ∾ "{ .type = fern_BQN_File_Constant_"‿𝕨‿", ."‿𝕨‿" = "‿(F 𝕩)‿"}"}¨

Comp ← ((< "{ .type = fern_BQN_File_Constant_runtime, .runtime = " ∾ "}" ∾˜ F)¨↕≠∾glyphs) glyphs _getComp ⊢
J ← ∾∾⟜(@+10)¨
Fconst ← ≡◶⟨@⊸≤◶{Num𝕩}‿Char, Str, ⊑⟩

MkBlock ← {𝕊 t‿i‿b :
  body ← {
    1=•Type 𝕩 ? < F 𝕩
   ;
    c ← ⌈´≠¨𝕩
    ∾´ (-c)⊸↑¨ (c⥊<"(uint32_t)-1")⊸∾¨ F¨¨ 𝕩
  } b
  ∾ "{ .type = "‿(F t)‿", .imm = "‿(F i)‿", .bodies = (uint32_t[])"‿(L body)‿", .num_bodies = "‿(F ≠ body)‿"}"
}

MkBody ← { 𝕊 start‿vars‿names‿export :
  ∾ "{ .start = "‿(F start)‿", .vars = "‿(F vars)‿"}"
}

Fout ← {
  fns ← (≠↑⟨F, Fconst, MkBlock, MkBody, L F¨⟩˙) 𝕩
	fns {L𝕎¨𝕩}¨ 𝕩
}

Long ← {𝕊 bc‿consts‿blocks‿bodies :
  J ⟨
    "#include ""local.h"""
    "static const fern_BQN_File_Bytecode bytecode[] = " ⋄ bc ⋄ ";"
    "static const fern_BQN_File_Constant constants[] = " ⋄ consts ⋄ ";"
    "static const fern_BQN_File_Block blocks[] = " ⋄ blocks ⋄ ";"
    "static const fern_BQN_File_Body bodies[] = " ⋄ bodies ⋄ ";"
    "fern_BQN_File module = {"
    "    .bytecode = bytecode"
    "  , .bytecode_size = sizeof(bytecode)"
    "  , .constants = constants"
    "  , .constants_size = sizeof(constants)"
    "  , .blocks = blocks"
    "  , .blocks_size = sizeof(blocks)"
    "  , .bodies = bodies"
    "  , .bodies_size = sizeof(bodies)"
    "};"
  ⟩
}
LFC ← Long∘Fout∘Comp

RT ← {
  src‿need‿inputs←𝕩•Import path∾"/src/pr.bqn"
  #pr ← "runtime_0"‿"provide" {(∾𝕨<⊸(<∘Ind⟜F¨)⟜(↕≠)¨𝕩)⊏˜(∾𝕩)⊐∾need}○((-1+1=𝕩)⊸↑)inputs
  pr ← "runtime_0"‿"provide" {
    (∾ 𝕨 <⊸ConstImport⟜(↕≠)¨ 𝕩) ⊏˜ (∾𝕩) ⊐ ∾ need
  }○((-1+1=𝕩)⊸↑)inputs
  #•Out •Repr pr
  Long Fout pr (need _getComp) src
}
CArg ← { J (¯5⊸↓∾𝕩˙)⌾⊑ •FLines path∾"/src/c.bqn"}
SVG ← {∾⟨"Modify←GetHighlights←⊢⋄"⟩∾ •FChars∘∾⟜".bqn"¨ "../svg"‿𝕩}

•Out (⊑"r"‿"r0"‿"r1"‿"c"‿"cc"‿"f"‿"e"‿"p"⊐⊏)◶⟨
  RT∘2, RT∘0, RT∘1
  {𝕩⋄LFC CArg "⟨"∾"⟩"«∾","⊸∾¨'"'(⊣∾∾˜)¨glyphs}
  {𝕩⋄LFC "{"∾"}"∾˜CArg"𝕩"}
  {𝕩⋄LFC •FChars path∾"/src/f.bqn"}
  {𝕩⋄LFC SVG "e"}
  {𝕩⋄LFC SVG "p"}
  ¯1 ↓ · J L∘Fout∘Comp¨
⟩ args
