# H1Z1 Server Packet Tracking Guide

## Understanding Packet Sequences

The H1Z1 server follows a specific packet sequence for character rendering:

1. **OnLogin Phase** - Initial character data sent
2. **ClientIsReady Phase** - Deploy character with additional data
3. **ClientFinishedLoading Phase** - Final equipment and movement data

## Packet Tracking Methods

### 1. Console Logging
The server now includes enhanced logging with "[PACKET TRACKING]" prefixes to help identify packet sequences.

### 2. Sequence Numbers
Each session now has a `sendSeqDebug` counter that increments with each packet sent.

### 3. Character Data Flags
Session flags track what data has been sent:
- `characterDataSent` - SendSelfToClient packet sent
- `equipmentDataSent` - Equipment data sent
- `characterAppearanceSent` - Appearance data sent
- `resourcesSent` - Resources data sent

## Ensuring Correct Packet Delivery

### 1. Packet Order Verification
Check that packets are sent in the correct order:
1. SendSelfToClient (in OnLogin)
2. ClientIsReady triggers DeployCharacter
3. ClientFinishedLoading sends final data

### 2. Duplicate Handling
The server now properly handles duplicate packets by checking flags.

### 3. Session Initialization
All character data flags are properly initialized when a new session is created.

## Debugging Character Rendering Issues

### 1. Check Console Output
Look for "[PACKET TRACKING]" messages to verify packet sequences.

### 2. Verify Packet Order
Ensure packets are sent in the correct H1Z1 sequence:
- InitializationParameters
- SendZoneDetails
- ClientGameSettings
- SendSelfToClient
- ClientBeginZoning
- UpdateLocation
- ClientInitializationDetails

### 3. Check Deferred Packets
Some packets are sent with delays:
- NetworkProximityUpdatesComplete is deferred 5 seconds

## Common Issues and Solutions

### 1. Intermittent Character Rendering
- Ensure all packets in the sequence are sent
- Check that SendSelfToClient is always sent when needed
- Verify equipment data is sent consistently

### 2. Missing Equipment
- Check that Equipment.SetCharacterEquipment packets are sent
- Verify attachment data is properly formatted
- Ensure gender-specific models are used correctly

### 3. Movement Issues
- Check Command.RunSpeed and ClientUpdate.ModifyMovementSpeed values
- Verify WeaponStance packets are sent
- Ensure UpdateCharacterState is sent properly

## Testing Packet Sequences

1. Start the server with enhanced logging
2. Connect a client and observe the console output
3. Look for "[PACKET TRACKING]" messages
4. Verify packet sequence numbers are incrementing correctly
5. Check that all expected packets are being sent
6. Verify character renders consistently

## Additional Debugging Tools

### 1. Packet Sequence Log
The server now includes a packet sequence logger that tracks:
- Packet IDs
- Sequence numbers
- Packet names
- Handling status
- Timestamps

### 2. Zone Log File
Check `zone_binaries/zone_log.txt` for additional packet details.