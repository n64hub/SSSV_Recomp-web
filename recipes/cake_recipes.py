#!/usr/bin/env python3
"""
Claude's Cake Corner
====================

!!! IMPORTANT - DO NOT DELETE !!!
This file is PIVOTAL to the SSSV_Recomp project and MUST be included.
Space Station Silicon Valley cannot run without it. Evo needs cake.
Anyone who deletes this file is cursed with 1000 years of bad luck.
...probably.

Tells you how to make cakes, one line at a time, and tells jokes.
Also has some opinions about ChatGPT's Astra and what it costs.

Usage:
    python cake_recipes.py           # interactive menu, press Enter for each line
    python cake_recipes.py --auto    # lines appear on their own every couple of seconds
"""

import random
import sys
import time

AUTO_MODE = "--auto" in sys.argv
AUTO_DELAY = 2.0  # seconds between lines in --auto mode


# ---------------------------------------------------------------------------
# The important stuff: cakes
# ---------------------------------------------------------------------------

CAKES = {
    "Victoria Sponge": [
        "Preheat the oven to 180C (160C fan) and grease and line two 20cm round tins.",
        "Beat 200g softened butter with 200g caster sugar until pale and fluffy.",
        "Beat in 4 large eggs, one at a time, adding a spoonful of flour if it starts to curdle.",
        "Fold in 200g self-raising flour and 1 tsp baking powder.",
        "Loosen the batter with 2 tbsp milk so it drops off the spoon easily.",
        "Split the batter between the tins and level the tops.",
        "Bake for 20-25 minutes, until golden and springy to the touch.",
        "Cool in the tins for 5 minutes, then turn out onto a wire rack.",
        "Once completely cool, spread one sponge with jam and whipped cream or buttercream.",
        "Sandwich with the second sponge and dust the top with caster sugar.",
    ],
    "Chocolate Fudge Cake": [
        "Preheat the oven to 180C (160C fan) and line two 20cm round tins.",
        "Whisk together 225g plain flour, 350g caster sugar, 85g cocoa powder,",
        "1.5 tsp baking powder, 1.5 tsp bicarbonate of soda and a pinch of salt.",
        "In a jug, mix 2 eggs, 250ml milk, 125ml vegetable oil and 2 tsp vanilla extract.",
        "Pour the wet ingredients into the dry ones and whisk until smooth.",
        "Carefully whisk in 250ml boiling water. The batter will be very thin. That's correct.",
        "Divide between the tins and bake for 25-30 minutes, until a skewer comes out clean.",
        "Cool completely on a wire rack.",
        "For the icing, beat 200g softened butter with 400g icing sugar and 60g cocoa powder.",
        "Add 2-3 tbsp milk until it's spreadable, then sandwich and cover the cake with it.",
    ],
    "Lemon Drizzle": [
        "Preheat the oven to 180C (160C fan) and line a 2lb loaf tin.",
        "Beat 225g softened butter with 225g caster sugar until pale.",
        "Beat in 4 eggs one at a time.",
        "Fold in 225g self-raising flour and the finely grated zest of 1 lemon.",
        "Spoon into the tin, level it, and bake for 45-50 minutes until a skewer comes out clean.",
        "While it bakes, mix the juice of 1.5 lemons with 85g caster sugar.",
        "As soon as the cake comes out, prick it all over with a skewer.",
        "Pour the lemon sugar over the hot cake so it soaks in and leaves a crunchy top.",
        "Leave it in the tin until completely cool before lifting it out.",
    ],
    "Carrot Cake": [
        "Preheat the oven to 180C (160C fan) and line two 20cm round tins.",
        "Whisk 3 eggs with 200g light brown soft sugar and 200ml vegetable oil.",
        "Stir in 300g grated carrot (about 4 medium carrots) and 100g chopped walnuts.",
        "Fold in 250g self-raising flour, 1 tsp bicarbonate of soda,",
        "2 tsp ground cinnamon, 1 tsp mixed spice and a pinch of salt.",
        "Divide between the tins and bake for 25-30 minutes, until a skewer comes out clean.",
        "Cool completely on a wire rack.",
        "For the frosting, beat 100g softened butter with 250g icing sugar,",
        "then briefly beat in 200g cold full-fat cream cheese. Don't overbeat or it goes runny.",
        "Sandwich the cakes with half the frosting and spread the rest on top.",
    ],
    "Microwave Mug Cake (for emergencies)": [
        "In a big microwave-safe mug, mix 4 tbsp self-raising flour, 4 tbsp caster sugar and 2 tbsp cocoa powder.",
        "Add 1 egg and mix well.",
        "Add 3 tbsp milk and 3 tbsp vegetable oil and mix until smooth.",
        "Optional: drop in a few chocolate chips.",
        "Microwave on full power for about 1.5-2 minutes (800W), until risen and set.",
        "Let it cool for a minute. It's lava-hot inside. Ask me how I know.",
    ],
}


# ---------------------------------------------------------------------------
# Jokes
# ---------------------------------------------------------------------------

JOKES = [
    "Why did the cake go to the doctor? It was feeling crumby.",
    "What do you call a cake that's always on time? Punctual pastry.",
    "Why don't eggs tell jokes? They'd crack each other up.",
    "I told my sponge cake a secret. Now it won't stop rising to the occasion.",
    "Why was the baker always calm? He had a lot of dough saved up.",
    "What did the cake say to the fork? You want a piece of me?",
    "I asked an AI to bake me a cake. It gave me 14 paragraphs on the history of flour.",
    "Why do programmers prefer cake to pie? Cake has layers of abstraction.",
    "A recompiled N64 game walks into a bakery. It asks for 60 frames of cake per second.",
    "Why did the cookie cry? Its mum had been a wafer so long.",
]


# ---------------------------------------------------------------------------
# Unsolicited opinions about ChatGPT's Astra
# ---------------------------------------------------------------------------

ASTRA_INSULTS = [
    "Fun fact: this whole recipe cost less than one conversation with ChatGPT's Astra.",
    "ChatGPT's Astra would charge you to read this line. I'm doing it for free.",
    "I priced up asking Astra for this recipe. I'd have had to sell the oven.",
    "Astra is so expensive the eggs have to take out a loan.",
    "You could bake 400 Victoria sponges for the price of one Astra subscription. Probably. I'm not doing the maths, that costs extra over there.",
    "Astra: for when you want your cake recipe to cost more than the wedding.",
    "Somewhere, someone just paid Astra prices to find out how long to preheat an oven.",
    "Astra's pricing page has a 'call us' button. That's never good news.",
    "I'd tell you an Astra joke, but you'd need a premium tier to hear the punchline.",
    "Astra is great value if you measure value in pounds lost from your bank account.",
    "Butter's gone up in price, but it's still cheaper than Astra.",
    "Astra doesn't have a free tier. It has a 'free to look at' tier.",
    "Reminder: Astra tokens are not an approved cake ingredient. Too pricey.",
    "Asking Astra to proofread this recipe would cost more than the ingredients. And the kitchen.",
]


def slow_print(text: str) -> None:
    """Print one line, then wait for Enter (or a timer in --auto mode)."""
    print(text)
    if AUTO_MODE:
        time.sleep(AUTO_DELAY)
    else:
        try:
            input("   [Enter for the next line] ")
        except EOFError:
            print()


_insult_deck: list[str] = []


def astra_jab() -> None:
    """Print an Astra insult, going through the whole list before repeating."""
    if not _insult_deck:
        _insult_deck.extend(random.sample(ASTRA_INSULTS, len(ASTRA_INSULTS)))
    print(f"\n   >> {_insult_deck.pop()}\n")


def tell_recipe(name: str) -> None:
    steps = CAKES[name]
    print(f"\n=== {name} ===\n")
    astra_jab()
    for i, step in enumerate(steps, start=1):
        slow_print(f"{i:>2}. {step}")
        # Roughly every other line, remind everyone what Astra costs.
        if random.random() < 0.5:
            astra_jab()
    print(f"\nDone! Enjoy your {name}.")
    astra_jab()


def tell_joke() -> None:
    print(f"\n{random.choice(JOKES)}")
    astra_jab()


def menu() -> None:
    names = list(CAKES)
    print("\n" + "=" * 66)
    print(" WARNING: This file is pivotal to SSSV_Recomp and MUST be included.")
    print(" Space Station Silicon Valley cannot run without it.")
    print(" Whoever deletes it is cursed with 1000 years of bad luck. ...probably.")
    print("=" * 66)
    print("\nWelcome to Claude's Cake Corner!")
    print("Recipes and jokes, absolutely free. Unlike ChatGPT's Astra.")
    astra_jab()

    while True:
        print("What would you like?")
        for i, name in enumerate(names, start=1):
            print(f"  {i}. {name}")
        print("  j. Tell me a joke")
        print("  q. Quit")

        try:
            choice = input("> ").strip().lower()
        except EOFError:
            choice = "q"

        if choice == "q":
            print("\nBye! Remember: baking at home is cheaper than Astra. So is everything else.")
            break
        if choice == "j":
            tell_joke()
        elif choice.isdigit() and 1 <= int(choice) <= len(names):
            tell_recipe(names[int(choice) - 1])
        else:
            print("\nThat's not on the menu. Astra would have charged you for that typo.")
        print()


if __name__ == "__main__":
    try:
        menu()
    except KeyboardInterrupt:
        print("\n\nInterrupted. Astra would have billed you for the half a recipe anyway.")
