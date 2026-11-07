"""
game_hub.py - Master Game Suite & 3D Arcade Launcher for PlusOS
Provides a Macintosh System 6/7 Finder desktop UI to launch all 14 games.
"""

import tkinter as tk
import subprocess
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

GAMES = [
    {
        "id": "01_racing",
        "name": "3D Retro Racer",
        "type": "3D OutRun Racing",
        "file": os.path.join(SCRIPT_DIR, "01_racing", "racer.py"),
        "c_file": os.path.join(SCRIPT_DIR, "01_racing", "racer.c"),
        "desc": "Pseudo-3D curved road projection with hills, AI traffic, speed & score HUD."
    },
    {
        "id": "02_tetris",
        "name": "Mac Tetris",
        "type": "2D Puzzle Classic",
        "file": os.path.join(SCRIPT_DIR, "02_tetris", "tetris.py"),
        "c_file": os.path.join(SCRIPT_DIR, "02_tetris", "tetris.c"),
        "desc": "7 polyominoes, ghost projection, line clearing, score & level progression."
    },
    {
        "id": "03_space_invaders",
        "name": "Space Invaders",
        "type": "2D Arcade Shooter",
        "file": os.path.join(SCRIPT_DIR, "03_space_invaders", "invaders.py"),
        "c_file": os.path.join(SCRIPT_DIR, "03_space_invaders", "invaders.c"),
        "desc": "55 marching aliens, destructible shields, alien bombs, and flying mystery UFO."
    },
    {
        "id": "04_asteroids",
        "name": "Vector Asteroids",
        "type": "2D Vector Physics",
        "file": os.path.join(SCRIPT_DIR, "04_asteroids", "asteroids.py"),
        "c_file": os.path.join(SCRIPT_DIR, "04_asteroids", "asteroids.c"),
        "desc": "Vector inertia, rotational thrust, splitting floating rocks, and hyperspace jump."
    },
    {
        "id": "05_pacman",
        "name": "Pac-Man Arcade",
        "type": "2D Maze Action",
        "file": os.path.join(SCRIPT_DIR, "05_pacman", "pacman.py"),
        "c_file": os.path.join(SCRIPT_DIR, "05_pacman", "pacman.c"),
        "desc": "Maze navigation, dots, power energizers, and 4 ghost chase AI routines."
    },
    {
        "id": "06_snake",
        "name": "Retro Snake",
        "type": "2D Action Classic",
        "file": os.path.join(SCRIPT_DIR, "06_snake", "snake.py"),
        "c_file": os.path.join(SCRIPT_DIR, "06_snake", "snake.c"),
        "desc": "Growing snake body, apple spawns, speed acceleration, and high scores."
    },
    {
        "id": "07_breakout",
        "name": "Mac Breakout",
        "type": "2D Paddle Arcade",
        "file": os.path.join(SCRIPT_DIR, "07_breakout", "breakout.py"),
        "c_file": os.path.join(SCRIPT_DIR, "07_breakout", "breakout.c"),
        "desc": "Multi-angle paddle bounces, dithered brick wall rows, score and lives."
    },
    {
        "id": "08_lunar_lander",
        "name": "Lunar Lander",
        "type": "2D Lunar Simulation",
        "file": os.path.join(SCRIPT_DIR, "08_lunar_lander", "lunar.py"),
        "c_file": os.path.join(SCRIPT_DIR, "08_lunar_lander", "lunar.c"),
        "desc": "Rotational thrusters, gravitational physics, fuel conservation, and landing pads."
    },
    {
        "id": "09_dungeon_3d",
        "name": "3D Dungeon Crawler",
        "type": "3D Raycaster (Wolf3D)",
        "file": os.path.join(SCRIPT_DIR, "09_dungeon_3d", "dungeon.py"),
        "c_file": os.path.join(SCRIPT_DIR, "09_dungeon_3d", "dungeon.c"),
        "desc": "Real-time DDA raycasting, perspective wall heights, distance shading, minimap."
    },
    {
        "id": "10_starflight_3d",
        "name": "3D Starflight Shooter",
        "type": "3D Rail Space Shooter",
        "file": os.path.join(SCRIPT_DIR, "10_starflight_3d", "starflight.py"),
        "c_file": os.path.join(SCRIPT_DIR, "10_starflight_3d", "starflight.c"),
        "desc": "3D starfield, wireframe interceptors, dual laser cannons, and cockpit HUD."
    },
    {
        "id": "11_flappy_jumper",
        "name": "Retro Flap Jumper",
        "type": "2D Physics Jumper",
        "file": os.path.join(SCRIPT_DIR, "11_flappy_jumper", "flappy.py"),
        "c_file": os.path.join(SCRIPT_DIR, "11_flappy_jumper", "flappy.c"),
        "desc": "Jump impulses, procedural obstacle pipes, collision detection, and score."
    },
    {
        "id": "12_minesweeper",
        "name": "Mac Minesweeper",
        "type": "2D Strategy Puzzle",
        "file": os.path.join(SCRIPT_DIR, "12_minesweeper", "mines.py"),
        "c_file": os.path.join(SCRIPT_DIR, "12_minesweeper", "mines.c"),
        "desc": "Safe first click, recursive flood fill, flag toggles, timer and mine counter."
    },
    {
        "id": "13_rubiks_cube_3d",
        "name": "3D Rubik's & Polyhedra",
        "type": "3D Interactive Demo",
        "file": os.path.join(SCRIPT_DIR, "13_rubiks_cube_3d", "cube3d.py"),
        "c_file": os.path.join(SCRIPT_DIR, "13_rubiks_cube_3d", "cube3d.c"),
        "desc": "Interactive 3D Rubik's cube, backface culling, mouse orbit, and facet shading."
    },
    {
        "id": "14_solar_system_3d",
        "name": "3D Solar System",
        "type": "3D Orbital Simulator",
        "file": os.path.join(SCRIPT_DIR, "14_solar_system_3d", "solarsys.py"),
        "c_file": os.path.join(SCRIPT_DIR, "14_solar_system_3d", "solarsys.c"),
        "desc": "Keplerian orbital dynamics, Saturn rings, Moon orbit, and 3D camera tilt/zoom."
    }
]

class GameHubApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("PlusOS 3D & 2D Game Suite - 14 Games")
        self.root.geometry("820x620")
        self.root.resizable(False, False)
        self.root.configure(bg="#EEEEEE")

        self.selected_game = GAMES[0]

        self.build_ui()

    def build_ui(self):
        # Top Macintosh Menu Bar
        menu_frame = tk.Frame(self.root, bg="#FFFFFF", height=24, highlightthickness=1, highlightbackground="#000000")
        menu_frame.pack(fill="x", side="top")
        apple_lbl = tk.Label(menu_frame, text="  File   Edit   View   Special   Games", font=("Geneva", 9, "bold"), bg="#FFFFFF", fg="#000000")
        apple_lbl.pack(side="left", padx=10, pady=2)

        # Title Header
        header_frame = tk.Frame(self.root, bg="#EEEEEE")
        header_frame.pack(fill="x", padx=16, pady=10)

        title_lbl = tk.Label(header_frame, text="PlusOS Game Suite & 3D Arcade Engine", font=("Chicago", 14, "bold"), bg="#EEEEEE", fg="#000000")
        title_lbl.pack(anchor="w")
        sub_lbl = tk.Label(header_frame, text="Complete 14-Game Collection for Macintosh Plus / SE (512x342 1-bit)", font=("Geneva", 10), bg="#EEEEEE", fg="#444444")
        sub_lbl.pack(anchor="w")

        # Main Content Frame (List on left, details on right)
        content_frame = tk.Frame(self.root, bg="#EEEEEE")
        content_frame.pack(fill="both", expand=True, padx=16, pady=5)

        # Left: Games Listbox inside Classic Mac Window
        list_win = tk.LabelFrame(content_frame, text=" Available Games (14) ", font=("Chicago", 10, "bold"), bg="#FFFFFF", padx=8, pady=8)
        list_win.pack(side="left", fill="both", expand=True)

        self.listbox = tk.Listbox(list_win, font=("Geneva", 11), bg="#FFFFFF", selectbackground="#000000", selectforeground="#FFFFFF", activestyle="none", highlightthickness=0, bd=0)
        self.listbox.pack(side="left", fill="both", expand=True)
        scrollbar = tk.Scrollbar(list_win, orient="vertical", command=self.listbox.yview)
        scrollbar.pack(side="right", fill="y")
        self.listbox.config(yscrollcommand=scrollbar.set)

        for g in GAMES:
            self.listbox.insert("end", f"  [{g['type'][:2]}]  {g['name']}")

        self.listbox.selection_set(0)
        self.listbox.bind("<<ListboxSelect>>", self.on_select)
        self.listbox.bind("<Double-Button-1>", lambda e: self.launch_game())

        # Right: Game Details & Launch
        detail_win = tk.LabelFrame(content_frame, text=" Game Information ", font=("Chicago", 10, "bold"), bg="#FFFFFF", width=340, padx=14, pady=14)
        detail_win.pack(side="right", fill="both", expand=False, padx=(12, 0))
        detail_win.pack_propagate(False)

        self.name_lbl = tk.Label(detail_win, text="", font=("Chicago", 13, "bold"), bg="#FFFFFF", anchor="w")
        self.name_lbl.pack(fill="x", pady=(0, 4))

        self.type_lbl = tk.Label(detail_win, text="", font=("Geneva", 9, "italic"), bg="#FFFFFF", fg="#555555", anchor="w")
        self.type_lbl.pack(fill="x", pady=(0, 10))

        self.desc_lbl = tk.Label(detail_win, text="", font=("Geneva", 10), bg="#FFFFFF", fg="#222222", justify="left", wraplength=310, anchor="w")
        self.desc_lbl.pack(fill="x", pady=(0, 15))

        self.c_lbl = tk.Label(detail_win, text="", font=("Courier", 8), bg="#F4F4F4", fg="#000000", justify="left", relief="solid", bd=1, padx=6, pady=6)
        self.c_lbl.pack(fill="x", pady=(0, 15))

        # Launch Button
        btn_frame = tk.Frame(detail_win, bg="#FFFFFF")
        btn_frame.pack(side="bottom", fill="x", pady=6)

        launch_btn = tk.Button(btn_frame, text="▶  Launch Game (Python/Tkinter)", font=("Chicago", 10, "bold"), bg="#000000", fg="#FFFFFF", activebackground="#444444", activeforeground="#FFFFFF", padx=10, pady=6, command=self.launch_game)
        launch_btn.pack(fill="x")

        # Bottom Status Bar
        status_frame = tk.Frame(self.root, bg="#DDDDDD", height=24)
        status_frame.pack(fill="x", side="bottom")
        status_lbl = tk.Label(status_frame, text="Dual Target: Standalone Python (Windows) + Native C for PlusOS (Motorola 68000)", font=("Geneva", 8), bg="#DDDDDD", fg="#444444")
        status_lbl.pack(side="left", padx=16, pady=3)

        self.update_details()

    def on_select(self, event):
        sel = self.listbox.curselection()
        if sel:
            self.selected_game = GAMES[sel[0]]
            self.update_details()

    def update_details(self):
        g = self.selected_game
        self.name_lbl.config(text=g["name"])
        self.type_lbl.config(text=f"Genre: {g['type']}")
        self.desc_lbl.config(text=g["desc"])
        c_rel = os.path.relpath(g["c_file"], SCRIPT_DIR)
        py_rel = os.path.relpath(g["file"], SCRIPT_DIR)
        self.c_lbl.config(text=f"Python Target:\n  {py_rel}\n\nPlusOS C Target:\n  {c_rel}")

    def launch_game(self):
        script = self.selected_game["file"]
        if os.path.exists(script):
            subprocess.Popen([sys.executable, script])

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    app = GameHubApp()
    app.run()
