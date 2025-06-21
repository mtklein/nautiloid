"""Text-based demo for Nautiloid Adventure."""

from dataclasses import dataclass, field
from typing import List, Callable


@dataclass
class Character:
    name: str
    char_class: str
    hp: int = 10

    def is_alive(self) -> bool:
        return self.hp > 0


@dataclass
class Companion(Character):
    pass


@dataclass
class Player(Character):
    companions: List[Companion] = field(default_factory=list)


@dataclass
class Room:
    name: str
    description: str
    action: Callable[[Player], None]

    def enter(self, player: Player) -> None:
        print(f"\n== {self.name} ==")
        print(self.description)
        self.action(player)


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


def pod_room(player: Player) -> None:
    print("You awaken in a slimy pod on a strange ship.")
    print("A wriggling creature looks at you expectantly. Attempt to calm it? (y/n)")
    if input("> ").lower().startswith("y"):
        player.companions.append(Companion(name="Familiar", char_class="Beast"))
        print("The creature chirps happily and follows you.")
    else:
        print("It scurries away into the shadows.")


def hallway(player: Player) -> None:
    print("An armored warrior is fending off small imps.")
    print("Help the warrior? (y/n)")
    if input("> ").lower().startswith("y"):
        ally = Companion(name="Warrior", char_class="Fighter")
        player.companions.append(ally)
        print("Together you defeat the imps!")
    else:
        print("You slip past, leaving the warrior to fight alone.")


def control_room(player: Player) -> None:
    print("A cleric struggles with the controls as the ship shudders.")
    print("Assist in stabilizing the craft? (y/n)")
    if input("> ").lower().startswith("y"):
        cleric = Companion(name="Cleric", char_class="Healer")
        player.companions.append(cleric)
        print("The ship rights itself just long enough for an escape pod to open.")
    else:
        print("Alarms blare as the vessel loses altitude rapidly!")
    print("You leap into the escape pod and launch away just in time.")


def main() -> None:
    print("Welcome to the Nautiloid Adventure!")
    name = input("Enter your character's name: ")
    char_class = choose_class()
    player = Player(name=name, char_class=char_class)

    rooms = [
        Room("Pod Room", "Gooey pods line the walls.", pod_room),
        Room("Hallway", "The corridor shakes with turbulence.", hallway),
        Room("Control Room", "Sparks fly from strange machinery.", control_room),
    ]

    for room in rooms:
        room.enter(player)
        if not player.is_alive():
            print("You have perished.")
            return

    print("\nYou and your companions have survived the ordeal!")
    if player.companions:
        print("Companions:")
        for comp in player.companions:
            print(f" - {comp.name} the {comp.char_class}")


if __name__ == "__main__":
    main()
