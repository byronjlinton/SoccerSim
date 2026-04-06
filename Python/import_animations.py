#!/usr/bin/env python
"""
Unreal Engine 5 Animation Import Script for SoccerSim
This script automates the import of Mixamo FBX animations and UE5 using the Editor Utility Widget.

USAGE:
1. Open UE5 Editor
2. Tools -> Execute Python Script or use Editor Utility Widget
3. Run this script

REQUIREMENTS:
- UE5 with Python plugin enabled
- IK Rig plugin enabled
- Animation Modifiers plugin enabled
"""

import unreal
import os

# Configuration
FBX_SOURCE_DIR = r"C:\Users\byron\.openclaw\Soccer"  # Source FBX files
CONTENT_DIR = "/Game/Anim/Mixamo"  # UE5 content path
RETARGETED_DIR = "/Game/Anim/Retargeted"  # Output path forIK_RIG_DIR = "/Game/Anim/IK"

# Animation mappings
ANIMATION_MAPPINGS = [
    {"fbx": "Offensive Idle.fbx", "anim": "AM_OffensiveIdle"},
    {"fbx": "Jog Forward.fbx", "anim": "AM_JogForward"},
    {"fbx": "Jog Backward.fbx", "anim": "AM_JogBackward"},
    {"fbx": "Jog Strafe Left.fbx", "anim": "AM_JogStrafeLeft"},
    {"fbx": "Jog Strafe Right.fbx", "anim": "AM_JogStrafeRight"},
    {"fbx": "Jog Backward Diagonal.fbx", "anim": "AM_JogBackwardDiagonal"},
    {"fbx": "Dribble.fbx", "anim": "AM_Dribble"},
    {"fbx": "Kick Soccerball.fbx", "anim": "AM_KickSoccerball"},
    {"fbx": "Soccer Header.fbx", "anim": "AM_SoccerHeader"},
    {"fbx": "Soccer Spin.fbx", "anim": "AM_SoccerSpin"},
    {"fbx": "Strike Foward Jog.fbx", "anim": "AM_StrikeForwardJog"},
]

@unreal.uclass()
class AnimationImporter(unreal.ScopedEditorUtilityWidget):
    def __init__(self, object):
        super().__init__(object)
        
    def button_import_animations_clicked(self):
        """Import all FBX animations from source directory"""
        unreal.log("Starting animation import...")
        
        # Import FBX files
        import_task = unreal.AssetImportTask()
        import_task.set_editor_property('filename', FBX_SOURCE_DIR)
        import_task.set_editor_property('destination_path', CONTENT_DIR)
        
        # Configure import options
        import_task.set_editor_property('automated', False)
        import_task.set_editor_property('save', True)
        
        # FBX import options - import only animations,        options = import_task.get_editor_property('options')
        options.set_editor_property('import_mesh', False)
        options.set_editor_property('import_as_skeletal', False)
        options.set_editor_property('import_animations', True)
        options.set_editor_property('override_full_name', True)
        
        # Animation import settings
        anim_options = options.get_editor_property('anim_sequence_import_data')
        anim_options.set_editor_property('animation_length', unreal.AnimationLength.EXPORTED_TIME)
        anim_options.set_editor_property('use_default_sample_rate', True)
        anim_options.set_editor_property('convert_scene', True)
        
        # Execute import
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([import_task])
        
        unreal.log("Animation import complete!")
        
    def button_create_ik_rigs_clicked(self):
        """Create IK Rigs for Mixamo and MetaHuman"""
        unreal.log("Creating IK Rigs...")
        
        # This would require manual setup in Editor
        # The Python API for IK Rig creation is complex and requires
        # interaction with the skeleton assets
        
        unreal.log_warning("IK Rig creation requires manual setup in Editor. Please follow the guide steps 3-5.")
        
    def button_retarget_animations_clicked(self):
        """Retarget animations from Mixamo to MetaHuman"""
        unreal.log("Retargeting animations...")
        
        # This would require the IK Retargeter to be set up first
        # and then call the retarget function
        
        unreal.log_warning("Animation retargeting requires IK Retargeter setup. Please follow the guide step 6.")
        
    def button_create_blendspace_clicked(self):
        """Create 8-directional blendspace"""
        unreal.log("Creating blendspace...")
        
        # This would require the retargeted animations to exist first
        # and needs manual setup
        
        unreal.log_warning("Blendspace creation requires retargeted animations. Please follow the guide step 7.")

def main():
    # When run as a script (not as widget), just log instructions
    unreal.log("=== SoccerSim Animation Import Script ===")
    unreal.log("")
    unreal.log("This script should be run as an Editor Utility Widget.")
    unreal.log("")
    unreal.log("To use:")
    unreal.log("1. Open Editor Utility Widget (Window -> Editor Utility Widget)")
    unreal.log("2. Create a new widget and add this script")
    unreal.log("3. Click the buttons to execute each step")
    unreal.log("")
    unreal.log("ALTERNATIVE: Use the manual steps in the guide at:")
    unreal.log("  C:\\Users\\byron\\Cursor\\SoccerSim\\Docs\\Animation-Editor-StepByStep.md")
    unreal.log("")
    unreal.log("Current state:")
    unreal.log(f"  FBX files: {FBX_SOURCE_DIR} ({len(ANIMATION_MAPPINGS)} files)")
    unreal.log(f"  Import to: {CONTENT_DIR}")
    unreal.log(f"  Retarget to: {RETARGETED_DIR}")

if __name__ == "__main__":
    main()
