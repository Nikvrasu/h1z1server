@echo off
echo ========================================
echo H1Z1 Server Packet Sequence Debug Tool
echo ========================================
echo.
echo This tool helps track packet sequences and debug character rendering issues.
echo.
echo To use this tool:
echo 1. Run the server with enhanced logging
echo 2. Look for "[PACKET TRACKING]" messages in the console output
echo 3. Check the sequence numbers to ensure packets are being sent in the correct order
echo.
echo Key packet sequence for character rendering:
echo - OnLogin sends SendSelfToClient and equipment data
echo - ClientIsReady triggers DeployCharacter
echo - ClientFinishedLoading sends final equipment and movement data
echo.
echo Common issues to look for:
echo - Missing packet sequence numbers
echo - Packets sent out of order
echo - Duplicate packet handling
echo - Missing character data flags
echo.
echo To enable more detailed logging:
echo - Set dump flags to TRUE in ConnectionArgs
echo - Check zone_log.txt for packet details
echo.
pause