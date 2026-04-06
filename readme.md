# Asteroid Rush

This is a prototype of a Gold Rush type of game with basic shape graphics.
It is a mix of tycoon and top down shooter with puzzle elements.

Gameplay is broken in 2 parts:
- Mission mode - top down view of a starship that collects resources and fights enemies, trying to run away with as much loot as possible
- Base mode - processing resources, crafting, selling them on dynamic market with dynamic pricing, filling in quest quotas and upgrading and stocking ship for further missions

## Mission mode

Top down controls, we control a ship with arrow controls. Environment is procedurally generated asteroids. Each asteroid is a physics object consisting of a tilemap. Each tile has different properties. 
Explosion resistance - how easily tile of this type gets destroyed by explosions
HP - number of health points of tile
Armor level - all bullets have a penetration level. If penetration level is lower than armor level bullet will either recoche or be absorbed into HP damage
Integrity - probabilty of either splitting into a separate body or becoming dust
Ricoche chance - chance of recoche or being absorbed by HP

Startship has buttons 1-9 for different weapon types
- Shrapnel splitting bullets with timer
- Regular shots
- High armor pernetration shots
- Explosive shells
- CLuster bombs
- Timed explosions
- Special weapon - catcher - when shot catches small parts of mineral or material. Automatically return them to the ship

Asteroids are consiting of gold, copper, silicon and other materials with different combinations of stats. The more valuable the material the harder to get it is in and closer to the core of asteroids.

Asteroid, apart from the most outward cells are hidden behind fog of war. You need to do scanning to detect certain materials. Detection is done by shooting intel drones that stick to surface and do a scan of local area. There is a limited amount of those. 

When tiles of asteroid get destroyed, they have a chance of getting transformed into dust, which obsures the view. This should be done with fluid simulation. Destroyed chunks add to fluid simulation and then it simulates it futher, this simulation should be rendered above asteroids and other objects to obscure them. 

Also the startship should have a 2d lighting system with a flashlight like system in front. Everything else should be dark. One of the guns should be temporary limited amount of flares to shoot out.

When done, player can turn on FTL, that after a while ends the mission with all collected resources saved for later base mode.

## Base mode

In base mode, we should have an inventory of all the resources. We should have multiple ways of processing ores. There should be a semi randomized trading market, a system of quests with missions like "collect X of resource Y" and upgrades for the ship. Total money should be stacked among runs, as well as resources. If a player dies on the mission, he loses all resources he took and the ship itself. Player can get a default ship and resources for free all the time, but they should be bad. Advanced stuff should be hard to buy or crafted. 