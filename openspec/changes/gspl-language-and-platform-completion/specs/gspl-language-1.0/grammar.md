# GSPL 1.0 Grammar (EBNF)

## Notation

```
Production   → NonTerminal "→" Expression ";" .
Expression   → Sequence ( "|" Sequence )* .
Sequence     → Term+ .
Term         → NonTerminal | Terminal | Group | Option | Repetition .
Group        → "(" Expression ")" .
Option       → "[" Expression "]" .
Repetition   → "{" Expression "}" .
Terminal     → "'" character+ "'" | '"' character+ '"' .
NonTerminal  → identifier .
```

GSPL keywords are reserved; terminal strings are case-sensitive.  
Comments (`//` line, `/* */` block) are stripped during tokenization and are legal anywhere whitespace is legal.

---

## 1. Top-Level Structure

```
Module        → "module" Identifier ";" { Import } { Declaration } .
Import        → "import" ModulePath ["namespace" Identifier] ";" .
ModulePath    → Identifier { "." Identifier } .
Declaration   → EntityDecl | FormDecl | TransformationDecl
              | MorphologyDecl | AbilityDecl | ResourceDecl
              | GeneDecl | RightsDecl | GenericBlock .
```

---

## 2. Entity

```
EntityDecl    → "entity" Identifier "{" { EntityBody } "}" ";" .
EntityBody    → RightsDecl | GeneDecl | FormDecl | TransformationDecl
              | MorphologyDecl | AbilityDecl | ResourceDecl
              | GenericBlock .
```

---

## 3. Rights

```
RightsDecl    → "rights" Identifier Identifier ";" .
```

Production classification tokens:

| Classification | Code |
|----------------|------|
| `ORIGINAL_USER_CREATION` | `PUBLIC`, `RESEARCH_ONLY`, `PROHIBITED` |
| `ANONYMIZED` | `PUBLIC`, `RESEARCH_ONLY` |
| `THIRD_PARTY_LICENSED` | `PUBLIC`, `RESEARCH_ONLY` |

---

## 4. Genes

```
GeneDecl      → "gene" Identifier [ ":" GeneDeps ] "{" { Attribute } "}" ";" .
GeneDeps      → Identifier { "," Identifier } .
Attribute     → Identifier ":" LiteralValue .
```

Gene `kind` values recognized at runtime: `identity`, `classification`, `appearance`, `morphology`, `form`, `transformation`, `ability`, `rights`, `runtime`, `animation`, `collision`.

---

## 5. Forms

```
FormDecl      → "form" Identifier [ ":" Identifier ] "{" { Attribute } "}" ";" .
```

The optional `":" Identifier` extends a previously defined form.

Attributes:

| Key | Type | Description |
|-----|------|-------------|
| `resource_capacity` | integer | Maximum resource pool |
| `collision_scale` | fixed | Collision geometry multiplier |
| `ability_envelope` | fixed | Ability power multiplier |
| `max_health` | integer | Maximum hit points |
| `transformations` | string | Comma-separated transformation IDs |

---

## 6. Transformations

```
TransformationDecl → "transformation" Identifier Identifier Identifier
                     "{" { Attribute } "}" ";" .
```

Positional identifiers: `name from_form to_form`.

Attributes:

| Key | Type | Description |
|-----|------|-------------|
| `duration` | integer | Duration in ticks |
| `resource_cost` | integer | Resource cost to activate |

---

## 7. Morphology

```
MorphologyDecl → "morphology" "{" { Part } "}" ";" .
Part           → Identifier "{" { Attribute } "}" ";" .
```

Per-part attributes:

| Key | Type | Description |
|-----|------|-------------|
| `x`, `y`, `z` | integer/fixed | Position offset in mm |
| `size_x`, `size_y`, `size_z` | integer/fixed | Bounding box extent in mm |
| `color` | string | Hex color `"#RRGGBB"` |
| `parent` | string | Parent part name |
| `rotation_degrees` | fixed | Z-axis rotation |
| `emissive` | bool | Self-illuminated |
| `electrical_marking` | bool | Electrical detail |

---

## 8. Abilities

```
AbilityDecl   → "ability" Identifier "{" { Attribute } "}" ";" .
```

Attributes:

| Key | Type | Description |
|-----|------|-------------|
| `effect` | string | Effect identifier |
| `cost` | integer | Resource cost |
| `cooldown_ticks` | integer | Cooldown period |
| `active_ticks` | integer | Active duration |
| `origin_socket` | string | Socket ID for projectile origin |
| `speed_mm_per_tick` | fixed | Projectile speed |
| `collision_radius_mm` | fixed | Collision radius |
| `status_id` | string | Status effect ID |
| `status_duration_ticks` | integer | Status effect duration |

---

## 9. Resources

```
ResourceDecl  → "resource" Identifier Identifier "{" { Attribute } "}" ";" .
```

Positional identifiers: `name resource_type`.

Attributes:

| Key | Type | Description |
|-----|------|-------------|
| `min` | integer | Minimum value |
| `max` | integer | Maximum value |
| `initial` | integer | Starting value |

---

## 10. Generic Blocks

```
GenericBlock  → Keyword Identifier { Identifier }
               "{" { GenericBody } "}" ";" .
GenericBody   → Attribute | GenericBlock .
Keyword       → "socket" | "joint" | "material" | "palette"
              | "animation" | "behavior" | "collision"
              | "bone" | "clip" | "state" | "transition"
              | "rig" | "track" | "event" | "window"
              | "initial" | "part" | "runtime" .
```

Nested generic blocks (e.g., `track` inside `clip`) are supported recursively.

---

## 11. Literals

```
LiteralValue  → IntegerLiteral | FixedLiteral | StringLiteral
              | BooleanLiteral | ColorLiteral | DurationLiteral
              | DistanceLiteral | AngleLiteral | PercentageLiteral .
IntegerLiteral → [ "-" ] digit { digit } .
FixedLiteral  → [ "-" ] digit { digit } "." digit { digit } .
StringLiteral → '"' { character } '"' .
BooleanLiteral → "true" | "false" .
ColorLiteral  → "#" hexdigit hexdigit hexdigit hexdigit hexdigit hexdigit .
DurationLiteral → digit { digit } "t" .
DistanceLiteral → digit { digit } [ "." digit { digit } ] "mm" .
AngleLiteral   → digit { digit } [ "." digit { digit } ] "deg" .
PercentageLiteral → digit { digit } [ "." digit { digit } ] "%" .
```

---

## 12. Identifiers

```
Identifier    → ( letter | "_" ) { letter | digit | "_" | "-" } .
letter        → "A" … "Z" | "a" … "z" .
digit         → "0" … "9" .
```

Hyphenated identifiers (e.g., `left_ear`, `base_to_storm`) are permitted.

---

## 13. Reserved Keywords

```
module  import  entity    gene     trait    form
transformation  morphology  part    joint   socket
material  palette  animation  behavior  ability
resource  collision  rights  provenance  namespace
extends  override  remove   if       else
true     false     let      const    fn
```

---

## 14. Lexical Grammar (Token Classes)

```
Token          → Keyword | Identifier | LiteralValue | Delimiter | Operator .
Delimiter      → "{" | "}" | "(" | ")" | "[" | "]"
               | ";" | ":" | "," | "." .
Operator       → "+" | "-" | "*" | "/" | "%"
               | "==" | "!=" | "<" | ">" | "<=" | ">="
               | "=" | "!" | "&&" | "||" | "^"
               | "->" | "@" | "~" | "?" .
Comment        → "//" { character } newline
               | "/*" { character } "*/" .
```
