# ReShade Addon UI --- UI/UX Layout Specification

## 1. Overall Goal

Redesign the ReShade addon UI into a clean, modern settings interface
inspired by the provided reference screenshot.

The UI should be organized around **category tabs in a fixed left
sidebar**. Selecting a category changes the settings displayed in the
main content area.

The design should feel like a native graphics/mod configuration panel
rather than a webpage.

------------------------------------------------------------------------

## 2. Overall Layout

The application is divided into two primary regions:

``` text
┌──────────────────────────────────────────────────────────────────────┐
│ Header / Application Bar                                             │
├───────────────────┬──────────────────────────────────────────────────┤
│                   │                                                  │
│   LEFT SIDEBAR    │              MAIN CONTENT AREA                   │
│                   │                                                  │
│   Category tabs   │   Selected category title                       │
│                   │                                                  │
│   Shadows         │   ┌──────────────────────────────────────────┐   │
│   Indirect Light  │   │ Section                                  │   │
│   Reflections     │   │ Controls                                 │   │
│   Ambient Occl.   │   │                                          │   │
│   Post Processing │   └──────────────────────────────────────────┘   │
│   ...             │                                                  │
│                   │   ┌──────────────────────────────────────────┐   │
│                   │   │ Another section                          │   │
│                   │   │ Controls                                 │   │
│                   │   └──────────────────────────────────────────┘   │
│                   │                                                  │
└───────────────────┴──────────────────────────────────────────────────┘
```

### Sidebar

-   Fixed width.
-   Occupies the full vertical height below the header.
-   Contains the main configuration categories.
-   Category names should be short and immediately understandable.
-   Each category is a clickable tab/navigation item.
-   The currently selected category must have a clearly visible active
    state.
-   Sidebar should remain visually stable while the main content
    changes.
-   The sidebar should not contain the actual settings for the selected
    category.

### Main Content

-   Displays only the settings belonging to the currently selected
    category.
-   Has its own vertical scrolling if the category contains many
    settings.
-   Starts with a clear category title.
-   Settings are grouped into logical sections.
-   Sections should be visually separated without becoming excessively
    heavy or decorative.

------------------------------------------------------------------------

## 3. Category Navigation

Initial categories should be structured approximately as:

-   Shadows
-   Indirect Lighting
-   Reflections
-   Ambient Occlusion
-   Post Processing
-   Anti-Aliasing
-   Global / General
-   Debug / Advanced

The exact categories can be adjusted to match the actual addon features.

### Category interaction

When the user clicks a sidebar category:

1.  The active sidebar item changes.
2.  The main content is replaced with that category's settings.
3.  The sidebar remains in the same position.
4.  The main content scroll position should preferably be remembered per
    category, if practical.
5.  Switching categories should not reset any settings.

Do not create a separate window for each category.

------------------------------------------------------------------------

## 4. Header

The top of the UI should contain a compact application/header bar.

Possible contents:

-   Addon name
-   Version or game name
-   Optional global controls
-   Optional reset button

The header should be visually distinct from the sidebar and content area
but remain compact.

Avoid wasting vertical space with a large title/header.

------------------------------------------------------------------------

## 5. Main Category Header

At the top of the main content area:

``` text
Shadows
```

or:

``` text
Indirect Lighting
```

The title should clearly communicate which category is currently active.

Optional:

-   Short one-line description beneath the title.
-   Category-level reset button.

Do not repeat the category name excessively inside every section.

------------------------------------------------------------------------

## 6. Settings Sections

Settings should be grouped into rectangular sections/cards.

Example:

``` text
SHADOWS

┌─────────────────────────────────────────────────────────────┐
│ Shadow Quality                                              │
│                                                             │
│ Enable Shadows                              [ ON ]           │
│ Shadow Distance                            ─────●──  100    │
│ Shadow Intensity                           ───●────  1.00   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

Each section should have:

-   A small section heading.
-   Related controls underneath.
-   Consistent internal spacing.
-   Consistent alignment between labels and controls.

Sections should visually separate groups of related settings while
remaining compact.

------------------------------------------------------------------------

## 7. Control Alignment

Use a consistent two-column structure for settings:

``` text
SETTING LABEL                         CONTROL
```

For example:

``` text
Shadow Distance                      ─────────●────  100
Shadow Intensity                     ─────●────────  1.00
Enable Contact Shadows               [ ON ]
Shadow Method                        [ PCSS       ▼ ]
```

Labels should share the same left alignment.

Controls should share the same right-side alignment.

Avoid individually positioning controls in arbitrary locations.

------------------------------------------------------------------------

## 8. Supported Control Types

The UI should visually distinguish different types of settings.

### Boolean

Use a compact toggle:

``` text
Enable Feature                         [ ON ]
```

or:

``` text
Enable Feature                         [ OFF ]
```

The active state should be visually obvious.

### Slider

Use:

``` text
Setting Name                  ───────●──────  1.00
```

The slider track should have:

-   Inactive/dark track
-   Visible active portion
-   Clearly visible thumb
-   Numeric value displayed at the right

### Dropdown / Combo

Use a compact selectable field:

``` text
Shadow Method                 [ PCSS          ▼ ]
```

### Numeric Value

Numeric settings should generally use a slider when the value has a
sensible range.

The current numeric value should remain visible.

### Reset

Individual settings that can be reset may have a small reset icon/button
aligned near the control.

Category-level reset can be provided separately.

------------------------------------------------------------------------

## 9. Visual Hierarchy

The UI should have three clear hierarchy levels:

### Level 1 --- Navigation

Sidebar category names.

### Level 2 --- Category

Main title such as:

`Shadows`

### Level 3 --- Section

Section headers such as:

`Shadow Quality`

`Contact Shadows`

`Cascaded Shadows`

Individual settings should remain visually subordinate to section
headers.

------------------------------------------------------------------------

## 10. Active Sidebar State

The active category should be immediately recognizable.

Recommended visual treatment:

``` text
┌────────────────────┐
│  Shadows            │  ← active
└────────────────────┘
```

Use a stronger background/accent treatment for the selected item.

Inactive categories should remain subdued.

Do not make every sidebar item visually prominent.

------------------------------------------------------------------------

## 11. Spacing

Use a consistent spacing system throughout the UI.

Important relationships:

-   Small spacing between a label and its control.
-   Medium spacing between individual settings.
-   Larger spacing between sections.
-   Comfortable padding inside section containers.
-   Consistent sidebar item height.
-   Consistent left/right margins in the main content.

The interface should feel dense enough for a graphics configuration tool
without becoming cramped.

------------------------------------------------------------------------

## 12. Scrolling

The main settings area may contain more settings than can fit
vertically.

Therefore:

-   Sidebar remains fixed.
-   Header remains fixed if practical.
-   Main settings area scrolls vertically.
-   Section cards continue naturally below the viewport.
-   Avoid nested scroll areas unless necessary.

The user should be able to configure large categories without the
navigation disappearing.

------------------------------------------------------------------------

## 13. Responsive Behavior

The UI should primarily target desktop use.

The important relationship is:

``` text
Fixed sidebar + flexible content
```

The sidebar should not expand excessively when the window becomes wider.

The main content area should consume the remaining horizontal space.

Settings should remain aligned when the window is resized.

------------------------------------------------------------------------

## 14. Design Direction

The provided screenshot is a **visual reference**, not something to copy
literally.

Important characteristics to preserve:

-   Dark UI.
-   Blue accent color.
-   Compact technical/graphics-tool appearance.
-   Clear left navigation.
-   Rounded section containers.
-   Subtle borders.
-   Strong active-state indication.
-   Clear typography hierarchy.
-   Controls aligned consistently.
-   Minimal decorative elements.
-   High information density without visual clutter.

The UI should look appropriate for a graphics mod / ReShade
configuration tool.

Avoid:

-   Large marketing-style UI elements.
-   Excessive gradients.
-   Excessive shadows/glows.
-   Oversized controls.
-   Large empty spaces.
-   Web-dashboard styling.
-   Unnecessary animations.
-   Decorative elements that compete with the settings.

------------------------------------------------------------------------

## 15. Interaction Model

The fundamental interaction should be:

``` text
User opens addon
        ↓
Default category is selected
        ↓
User clicks a category in the sidebar
        ↓
Main content switches to that category
        ↓
User changes settings
        ↓
Settings remain active
        ↓
User switches to another category
```

The navigation should feel immediate and lightweight.

No page reloads or separate dialogs should be required for normal
settings.

------------------------------------------------------------------------

## 16. Important Implementation Principle

Do not treat each category as a completely separate UI.

The implementation should use a **shared UI framework/layout**:

``` text
Addon UI
├── Header
├── Sidebar
│   ├── Shadows
│   ├── Indirect Lighting
│   ├── Reflections
│   ├── Ambient Occlusion
│   ├── Post Processing
│   └── ...
└── Content Area
    └── Active Category
        ├── Section
        ├── Section
        └── Section
```

Changing the active category should primarily change the contents of the
content area.

This keeps spacing, control appearance, navigation behavior, and styling
consistent across the entire addon.

------------------------------------------------------------------------

## 17. Reference Screenshot Interpretation

The provided screenshot demonstrates the intended general structure:

-   A narrow vertical navigation area on the left.
-   A substantially larger settings/content area on the right.
-   Category title at the top of the content area.
-   Settings divided into rectangular sections.
-   Section headings using the blue accent.
-   Labels aligned on the left.
-   Controls aligned toward the right.
-   Sliders occupying a consistent horizontal region.
-   Numeric values displayed beside sliders.
-   Toggles and dropdowns using compact controls.
-   Dark background with subtle separation between panels.

The new UI should use these principles while adapting them to the actual
ReShade addon settings and available screen space.
