"""Text-based demo for Nautiloid Adventure."""

from dataclasses import dataclass, field
from typing import List, Callable, Dict, Optional
import random


@dataclass
class Ability:
    """Simple representation of an action in combat."""

    name: str
    target: str  # "enemy" or "ally"
    power: int


@dataclass
class Character:
    name: str
    char_class: str
    hp: int = 10
    abilities: List[Ability] = field(default_factory=list)

    def is_alive(self) -> bool:
        return self.hp > 0


@dataclass
class Companion(Character):
    pass


@dataclass
class Player(Character):
    companions: List[Companion] = field(default_factory=list)
    inventory: List[str] = field(default_factory=list)
    flags: Dict[str, bool] = field(default_factory=dict)


CLASS_ABILITIES: Dict[str, List[Ability]] = {
    "Fighter": [Ability("Strike", "enemy", 3), Ability("Power Attack", "enemy", 5)],
    "Rogue": [Ability("Stab", "enemy", 3), Ability("Sneak Attack", "enemy", 4)],
    "Mage": [Ability("Firebolt", "enemy", 4), Ability("Barrier", "ally", 3)],
    "Healer": [Ability("Smite", "enemy", 3), Ability("Heal", "ally", 4)],
    "Beast": [Ability("Bite", "enemy", 2), Ability("Encourage", "ally", 2)],
}


@dataclass
class Room:
    name: str
    description: str
    action: Callable[[Player], Optional[str]]

    def enter(self, player: Player) -> Optional[str]:
        print(f"\n== {self.name} ==")
        print(self.description)
        return self.action(player)


def choose_class() -> str:
    classes = ["Fighter", "Rogue", "Mage"]
    print("Choose your class:")
    for idx, c in enumerate(classes, 1):
        print(f"{idx}. {c}")
    while True:
        choice = input("> ")
        if choice.isdigit() and 1 <= int(choice) <= len(classes):
            return classes[int(choice) - 1]
        print("Invalid choice.")


def open_chest(player: Player, flag: str, item: str) -> None:
    if player.flags.get(flag):
        print("The chest is empty.")
    else:
        print(f"You open the chest and find {item}!")
        player.inventory.append(item)
        player.flags[flag] = True


def familiar_dialog() -> None:
    while True:
        print("The familiar chirps softly, awaiting your words.")
        print("1. \"Who are you?\"")
        print("2. \"Will you help me escape?\"")
        print("3. \"Let's go.\"")
        choice = input("> ")
        if choice == "1":
            print("It chitters about being bound to the ship by foul magic.")
        elif choice == "2":
            print("The creature nods enthusiastically.")
        elif choice == "3":
            print("It hops onto your shoulder.")
            return
        else:
            print("It stares in confusion.")


def warrior_dialog() -> None:
    while True:
        print("The warrior wipes ichor from his blade.")
        print("1. \"What's your name?\"")
        print("2. \"Stick with me, we can escape.\"")
        print("3. \"Enough talk.\"")
        choice = input("> ")
        if choice == "1":
            print("\"Call me whatever you like. Let's just survive,\" he grunts.")
        elif choice == "2":
            print("He nods. 'Lead the way.'")
        elif choice == "3":
            print("He falls in behind you, ready for battle.")
            return
        else:
            print("He raises an eyebrow, not understanding.")


def cleric_dialog() -> None:
    while True:
        print("The cleric steadies her breath, relieved.")
        print("1. \"What happened here?\"")
        print("2. \"Can you heal us?\"")
        print("3. \"Let's leave this ship.\"")
        choice = input("> ")
        if choice == "1":
            print("'A ritual went terribly wrong,' she explains.")
        elif choice == "2":
            print("She murmurs a prayer and a warm light surrounds you.")
        elif choice == "3":
            print("She grabs a nearby pack and prepares to follow.")
            return
        else:
            print("She looks puzzled by your response.")


def combat(player: Player, enemies: List[Character]) -> None:
    """Basic turn-based combat system."""
    allies = [player] + [c for c in player.companions if c.is_alive()]
    round_num = 1
    while player.is_alive() and any(e.is_alive() for e in enemies):
        print(f"\n-- Round {round_num} --")
        print("Allies:")
        for a in allies:
            if a.is_alive():
                print(f"  {a.name}: {a.hp} HP")
        print("Enemies:")
        for idx, e in enumerate(enemies, 1):
            if e.is_alive():
                print(f"  {idx}. {e.name}: {e.hp} HP")

        for ally in allies:
            if not any(e.is_alive() for e in enemies):
                break
            if not ally.is_alive():
                continue
            print(f"\n{ally.name}'s turn")
            for i, ab in enumerate(ally.abilities, 1):
                print(f"{i}. {ab.name}")
            while True:
                choice = input("> ")
                if choice.isdigit() and 1 <= int(choice) <= len(ally.abilities):
                    ability = ally.abilities[int(choice) - 1]
                    break
                print("Invalid choice.")
            if ability.target == "enemy":
                targets = [e for e in enemies if e.is_alive()]
                for i, t in enumerate(targets, 1):
                    print(f"{i}. {t.name} ({t.hp} HP)")
                while True:
                    t_choice = input("> ")
                    if t_choice.isdigit() and 1 <= int(t_choice) <= len(targets):
                        target = targets[int(t_choice) - 1]
                        break
                    print("Invalid choice.")
                target.hp -= ability.power
                print(f"{ally.name} uses {ability.name} on {target.name} for {ability.power} damage!")
            else:
                targets = [a for a in allies if a.is_alive()]
                for i, t in enumerate(targets, 1):
                    print(f"{i}. {t.name} ({t.hp} HP)")
                while True:
                    t_choice = input("> ")
                    if t_choice.isdigit() and 1 <= int(t_choice) <= len(targets):
                        target = targets[int(t_choice) - 1]
                        break
                    print("Invalid choice.")
                target.hp += ability.power
                print(f"{ally.name} uses {ability.name} on {target.name}, healing {ability.power} HP!")

        for enemy in enemies:
            if enemy.is_alive() and player.is_alive():
                target = random.choice([a for a in allies if a.is_alive()])
                ability = enemy.abilities[0] if enemy.abilities else Ability("Attack", "enemy", 2)
                target.hp -= ability.power
                print(f"{enemy.name} attacks {target.name} for {ability.power} damage!")
        round_num += 1

    if player.is_alive():
        print("You are victorious!")
    else:
        print("You were defeated...")


def pod_room(player: Player) -> str:
    if not player.flags.get("intro_shown"):
        print("You awaken in a slimy pod on a strange ship.")
        player.flags["intro_shown"] = True

    if not player.flags.get("familiar_met"):
        print("A wriggling creature eyes you curiously.")
        print("1. Calm the creature")
        print("2. Ignore it")
        choice = input("> ")
        if choice == "1":
            player.companions.append(
                Companion(
                    name="Familiar",
                    char_class="Beast",
                    abilities=CLASS_ABILITIES["Beast"].copy(),
                )
            )
            print("The creature chirps happily and follows you.")
            familiar_dialog()
        else:
            print("It scurries away into the shadows.")
        player.flags["familiar_met"] = True

    while True:
        print("\nActions:")
        opts = []
        if not player.flags.get("pod_chest_opened"):
            print("1. Open the chest")
            opts.append("1")
        print("2. Inspect the pods")
        print("3. Go east to the Corridor")
        choice = input("> ")
        if choice == "1" and "1" in opts:
            open_chest(player, "pod_chest_opened", "a strange dagger")
        elif choice == "2":
            print("The pods pulse with eerie light.")
        elif choice == "3":
            return "Corridor"
        else:
            print("Invalid choice.")


def corridor(player: Player) -> str:
    print("You stand in a dim corridor, the ship creaking around you.")
    while True:
        print("\nDoors lead to:")
        print("1. Pod Room (west)")
        print("2. Brig (north)")
        print("3. Storage (south)")
        print("4. Armory (east)")
        choice = input("> ")
        if choice == "1":
            return "Pod Room"
        if choice == "2":
            return "Brig"
        if choice == "3":
            return "Storage"
        if choice == "4":
            return "Armory"
        print("Invalid choice.")


def brig(player: Player) -> str:
    if not player.flags.get("warrior_met"):
        print("A warrior is trapped behind a sparking force field, battling imps.")
        print("1. Disable the field and help")
        print("2. Leave him")
        choice = input("> ")
        if choice == "1":
            warrior = Companion(
                name="Warrior",
                char_class="Fighter",
                abilities=CLASS_ABILITIES["Fighter"].copy(),
            )
            player.companions.append(warrior)
            print("Together you defeat the imps and free him!")
            warrior_dialog()
            player.flags["warrior_rescued"] = True
        else:
            print("You leave him struggling as you back away.")
        player.flags["warrior_met"] = True
    else:
        if player.flags.get("warrior_rescued"):
            print("The empty cell reminds you of the warrior's gratitude.")
        else:
            print("The warrior still battles on without your help.")
    input("Press Enter to return to the corridor.")
    return "Corridor"


def storage(player: Player) -> str:
    print("Crates spill their contents across the floor.")
    while True:
        print("\nActions:")
        opts = []
        if not player.flags.get("storage_chest_opened"):
            print("1. Open the supply chest")
            opts.append("1")
        print("2. Read the scribbled note")
        print("3. Return to the Corridor")
        choice = input("> ")
        if choice == "1" and "1" in opts:
            open_chest(player, "storage_chest_opened", "a healing salve")
        elif choice == "2":
            print("The note mentions experiments on unwilling subjects.")
        elif choice == "3":
            return "Corridor"
        else:
            print("Invalid choice.")


def armory(player: Player) -> str:
    if not player.flags.get("armory_cleared"):
        print("Imps rush from behind the weapon racks!")
        enemies = [
            Character("Imp", "Demon", 5, [Ability("Claw", "enemy", 2)]),
            Character("Imp", "Demon", 5, [Ability("Claw", "enemy", 2)]),
        ]
        combat(player, enemies)
        if not player.is_alive():
            return None
        player.flags["armory_cleared"] = True
    while True:
        print("\nActions:")
        print("1. Continue east to the Control Room")
        print("2. Return to the Corridor")
        choice = input("> ")
        if choice == "1":
            return "Control Room"
        if choice == "2":
            return "Corridor"
        print("Invalid choice.")


def control_room(player: Player) -> str:
    if not player.flags.get("cleric_met"):
        print("A cleric struggles with the controls as the ship shudders violently.")
        print("1. Help stabilise the craft")
        print("2. Ignore the cleric")
        choice = input("> ")
        if choice == "1":
            cleric = Companion(
                name="Cleric",
                char_class="Healer",
                abilities=CLASS_ABILITIES["Healer"].copy(),
            )
            player.companions.append(cleric)
            print("You manage to steady the vessel for a moment.")
            cleric_dialog()
            player.flags["cleric_rescued"] = True
        else:
            print("The alarms grow louder as you hesitate.")
        player.flags["cleric_met"] = True
    while True:
        print("\nActions:")
        print("1. Go east toward the Escape Pods")
        print("2. Return to the Corridor")
        choice = input("> ")
        if choice == "1":
            return "Escape Hallway"
        if choice == "2":
            return "Corridor"
        print("Invalid choice.")


def escape_hallway(player: Player) -> str:
    if not player.flags.get("hallway_cleared"):
        print("A crazed cultist blocks your path to the pods!")
        enemies = [Character("Cultist", "Enemy", 8, [Ability("Slash", "enemy", 3)])]
        combat(player, enemies)
        if not player.is_alive():
            return None
        player.flags["hallway_cleared"] = True
    while True:
        print("\nActions:")
        print("1. Enter the Escape Pod")
        print("2. Return to the Control Room")
        choice = input("> ")
        if choice == "1":
            return "Escape Pod"
        if choice == "2":
            return "Control Room"
        print("Invalid choice.")


def escape_pod(player: Player) -> Optional[str]:
    print("You strap into the escape pod and launch away from the nautiloid.")
    print("\nYou and your companions have survived the ordeal!")
    if player.companions:
        print("Companions:")
        for comp in player.companions:
            print(f" - {comp.name} the {comp.char_class}")
    if player.inventory:
        print("Inventory:")
        for item in player.inventory:
            print(f" - {item}")
    return None


def main() -> None:
    print("Welcome to the Nautiloid Adventure!")
    name = input("Enter your character's name: ")
    char_class = choose_class()
    player = Player(
        name=name,
        char_class=char_class,
        abilities=CLASS_ABILITIES[char_class].copy(),
    )

    rooms = {
        "Pod Room": Room("Pod Room", "Gooey pods line the walls.", pod_room),
        "Corridor": Room("Corridor", "Connecting passages of the ship.", corridor),
        "Brig": Room("Brig", "Cells hold unfortunate prisoners.", brig),
        "Storage": Room("Storage", "Stacks of supplies and tools.", storage),
        "Armory": Room("Armory", "Racks of weapons fill the room.", armory),
        "Control Room": Room("Control Room", "Sparks fly from strange machinery.", control_room),
        "Escape Hallway": Room("Escape Hallway", "The path to the pods.", escape_hallway),
        "Escape Pod": Room("Escape Pod", "A small pod ready for launch.", escape_pod),
    }

    current = "Pod Room"
    while current:
        current = rooms[current].enter(player)
        if not player.is_alive():
            print("You have perished.")
            return


if __name__ == "__main__":
    main()
