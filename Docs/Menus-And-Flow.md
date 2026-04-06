# Menus and Game Flow

This doc describes how to get a full game flow: main menu → match → pause → match end (Rematch / Main Menu).

---

## 1. Game Instance (C++)

The project has **USoccerGameInstance** which provides:

- **OpenMatchMap()** – opens the match level (name from config or default `Match`).
- **OpenMainMenuMap()** – opens the main menu level (default `MainMenu`).
- **Rematch()** – reopens the current level (same match map).

**Config (Optional):** In `Config/DefaultGame.ini` under `[/Script/SoccerSim.SoccerGameInstance]` you can set:

- `MatchMapName=Match` – level to load when starting a match.
- `MainMenuMapName=MainMenu` – level to load for main menu and when quitting to menu.

**Editor:** In **Project Settings → Maps & Modes → Game Instance Class**, set to **SoccerGameInstance** (or a Blueprint child).

---

## 2. Default map (main menu first)

In **Project Settings → Maps & Modes**, set **Game Default Map** to your main menu map (e.g. create **Content/Maps/MainMenu**) so that **Play** starts on the menu. Create **Content/Maps/Match** (or your match level) and set **MatchMapName** in config if different.

---

## 3. Main menu

1. Create a level **MainMenu** (no GameMode required for the menu).
2. Add a Widget to the viewport (or use a simple level with a **Start** and **Quit** button).
3. **Start** button: call **Get Game Instance** → cast to **Soccer Game Instance** → **Open Match Map**.
4. **Quit** button: call **Quit Game** (or **Open Main Menu Map** if you prefer to stay in-app).

The match level should have **World Settings → GameMode Override** = **BP_SoccerGameMode** (or use global default) so the full match runs.

---

## 4. Pause menu (Escape)

**SoccerPlayerController** binds **Escape** to toggle pause and show/hide the pause widget.

1. Create a **Widget Blueprint** (e.g. **WBP_PauseMenu**) with:
   - **Resume** button → get Player Controller (0), call **Resume Game**.
   - **Quit to Menu** button → get Player Controller (0), call **Quit To Main Menu**.
2. Open **BP_SoccerPlayerController** → **Class Defaults** → set **Pause Menu Widget Class** to **WBP_PauseMenu**.

When the player presses Escape during a match, the game pauses and the widget is shown; Resume removes it and unpauses; Quit to Menu opens the main menu map (via Game Instance).

---

## 5. Match end screen (Full Time)

When the match phase becomes **Full Time**, the controller automatically adds the **Match End** widget once (if set).

1. Create a **Widget Blueprint** (e.g. **WBP_MatchEnd**) with:
   - Score (e.g. get **Soccer Game State** → **Home Score** / **Away Score**).
   - **Rematch** button → get Player Controller (0), call **Rematch**.
   - **Main Menu** button → get Player Controller (0), call **Quit To Main Menu**.
2. Open **BP_SoccerPlayerController** → **Class Defaults** → set **Match End Widget Class** to **WBP_MatchEnd**.

---

## 6. Full flow checklist

- [ ] Game Instance Class = SoccerGameInstance (or BP child).
- [ ] Game Default Map = main menu map.
- [ ] Main menu map has Start (Open Match Map) and Quit.
- [ ] Match map uses BP_SoccerGameMode (or default).
- [ ] BP_SoccerPlayerController: Pause Menu Widget Class = WBP_PauseMenu, Match End Widget Class = WBP_MatchEnd.
- [ ] Optional: DefaultGame.ini `MatchMapName` / `MainMenuMapName` if level names differ.

After that: **Play** → main menu → Start → match → Escape to pause → Resume or Quit to Menu → when the match ends, match end widget → Rematch or Main Menu.
