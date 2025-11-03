# Trigger Blueprint Action User Guide

## Quick Start

### Observing Actions

1. Add the Trigger Blueprint action to a Stream Deck device
2. Add one of the 3 "Observe Action" Blueprint nodes
   - `Observe Key Action`
   - `Observe Dial Action`
   - `Observe Dial Value Action`
3. Drag from the desired delegate (red box) and select **"Add Custom Event..."**
4. From the `Custom Event` node, trigger the desired Blueprint logic

### Updating Actions

#### Within the Unreal Editor:

1. Add the `Bind Trigger Blueprint Action` node
2. Set an appropriate identifier value (this will appear in Stream Deck)
3. Promote the return value to a variable

> ⚠️ If the return value is not captured as a variable, the binding will be lost!

4. Pass the captured return value to the `Set Value` node

#### Within Stream Deck:

1. Add the Trigger Blueprint action to a Stream Deck device
2. With the game running, select an identifier from the list