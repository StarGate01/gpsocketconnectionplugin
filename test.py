#!/usr/bin/env python3

import os, socket, threading, argparse

import globalplatform as gp

import smartcard
from smartcard.System import readers
from smartcard.CardRequest import CardRequest
from smartcard.CardType import AnyCardType
from smartcard.ExclusiveConnectCardConnection import ExclusiveConnectCardConnection

ISD_AID = bytearray.fromhex("A000000151000000")


def list_applets(cardContext, cardInfo):
    # Select ISD
    gp.OPGP_select_application(cardContext, cardInfo, ISD_AID, len(ISD_AID))

    # Read secure channel parameters
    scp = bytearray(1)
    scpImpl = bytearray(1)
    gp.GP211_get_secure_channel_protocol_details(cardContext, cardInfo, scp, scpImpl)
    print(f"SCP: {hex(scp[0])}, Impl: {hex(scpImpl[0])}")

    # Perform authentication using default keyset
    key = bytearray.fromhex("404142434445464748494A4B4C4D4E4F")
    zero_key = bytearray(16)
    secInfo = gp.GP211_SECURITY_INFO()
    secInfo.invokingAid = ISD_AID
    secInfo.invokingAidLength = len(ISD_AID)
    gp.GP211_mutual_authentication(cardContext, cardInfo, 
        baseKey = zero_key, S_ENC = key, S_MAC = key, DEK = key, keyLength = len(key),
        keySetVersion = 0, keyIndex = 0, secureChannelProtocol = scp[0], secureChannelProtocolImpl = scpImpl[0],
        securityLevel = gp.GP211_SCP02_SECURITY_LEVEL_C_DEC_C_MAC_R_MAC,
        derivationMethod = gp.OPGP_DERIVATION_METHOD_NONE, secInfo = secInfo)

    # Query installed packages and applications
    executableData = gp.new_GP211_EXECUTABLE_MODULES_DATA_Array(64)
    applData = gp.new_GP211_APPLICATION_DATA_Array(64)
    dataLengthPtr = gp.new_DWORDp()
    # Query packages
    gp.DWORDp_assign(dataLengthPtr, 64)
    gp.GP211_get_status(cardContext, cardInfo,
        secInfo = secInfo, cardElement = gp.GP211_STATUS_LOAD_FILES_AND_EXECUTABLE_MODULES,
        format = 2, applData = applData, executableData = executableData, dataLength = dataLengthPtr)
    dataLengthExe = gp.DWORDp_value(dataLengthPtr)
    # Query applications
    gp.DWORDp_assign(dataLengthPtr, 64)
    gp.GP211_get_status(cardContext, cardInfo,
        secInfo = secInfo, cardElement = gp.GP211_STATUS_APPLICATIONS,
        format = 2, applData = applData, executableData = executableData, dataLength = dataLengthPtr)
    dataLengthAppl = gp.DWORDp_value(dataLengthPtr)
    gp.delete_DWORDp(dataLengthPtr)

    # Print list of packages
    print(f"Found {dataLengthExe} packages:")
    print("")
    for i in range(dataLengthExe):
        exe = gp.GP211_EXECUTABLE_MODULES_DATA_Array_getitem(executableData, i)
        print(f"Executable load file: {exe.aid.AID[:exe.aid.AIDLength].hex()}")
        print(f" - SD AID: {exe.associatedSecurityDomainAID.AID[:exe.associatedSecurityDomainAID.AIDLength].hex()}")
        print(f" - Version: {exe.versionNumber[0]}.{exe.versionNumber[1]}")
        print(f" - Lifecycle: {exe.lifeCycleState}")
        print(" - Executable modules:")
        for j in range(exe.numExecutableModules):
            module = gp.OPGP_AID_Array_getitem(exe.executableModules, j)
            print(f"   - {module.AID[:module.AIDLength].hex()}")
        print("")
    gp.delete_GP211_EXECUTABLE_MODULES_DATA_Array(executableData)
    # Print list of applications
    print(f"Found {dataLengthAppl} instances:")
    print("")
    for i in range(dataLengthAppl):
        app = gp.GP211_APPLICATION_DATA_Array_getitem(applData, i)
        print(f"Instance: {app.aid.AID[:app.aid.AIDLength].hex()}")
        print(f" - SD AID: {app.associatedSecurityDomainAID.AID[:app.associatedSecurityDomainAID.AIDLength].hex()}")
        print(f" - Version: {app.versionNumber[0]}.{app.versionNumber[1]}")
        print(f" - Lifecycle: {app.lifeCycleState}")
        print(f" - Privileges: {app.privileges}")
    print("")
    gp.delete_GP211_APPLICATION_DATA_Array(applData)

def apdu_proxy(card, sock, stop_event):
    try:
        sock.settimeout(0.5)
        while not stop_event.is_set():
            try:
                command = sock.recv(4096)
                if not command:
                    print("Connection closed by peer.")
                    break
                #print(f">> {command.hex()}")
                data, sw1, sw2 = card.connection.transmit(list(command))
                response = bytes(data + [sw1, sw2])
                #print(f"<< {response.hex()}")
                sock.send(response)

            except socket.timeout:
                continue
            except (ConnectionResetError, BrokenPipeError):
                print("Connection error, closing socket.")
                break
            except Exception as e:
                print(f"Unexpected error: {e}")
                break
    finally:
        sock.close()
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Test global platform unix socket plugin')
    parser.add_argument('-r', '--reader', nargs='?', dest='reader', type=str, required=True, help='PC/SC reader to use')
    args = parser.parse_args()

    # Enable debug logging to the console
    #os.environ["GLOBALPLATFORM_DEBUG"] = "1"
    #os.environ["GLOBALPLATFORM_LOGFILE"] = "/dev/stdout"

    # Wait for and connect to card via PC/SC
    print(f'info: Using reader {args.reader}, waiting for specified cards ... ', end='', flush=True)
    request = CardRequest(timeout=None, newcardonly=False, readers=[args.reader], cardType=AnyCardType())
    card = request.waitforcard()
    print('found a new card')
    card.connection = ExclusiveConnectCardConnection(card.connection)
    card.connection.connect()
    atr = card.connection.getATR()
    print(f'info: Found card with ATR: {bytes(atr).hex()}')

    # Read key diversification data (KDD ID)
    data, sw1, sw2 = card.connection.transmit([0x00, 0xa4, 0x04, 0x00, len(ISD_AID)] + list(ISD_AID))
    if(sw1 != 0x90 and sw2 != 0x00):
        raise Exception("Cannot select ISD")
    data, sw1, sw2 = card.connection.transmit([0x80, 0x50, 0x00, 0x00, 8] + list(bytearray(8)))
    if(sw1 != 0x90 and sw2 != 0x00):
        raise Exception("Cannot process initialize update")
    kdd = bytes(data[:10])
    print(f"KDD: {kdd.hex()}")

    # Open sockets and write ATR
    app_socket, plugin_socket = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
    app_socket.sendmsg([bytes(atr)])

    # Start ADPU proxy
    stop_event = threading.Event()
    proxy_thread = threading.Thread(target=apdu_proxy, args=(card, app_socket, stop_event), daemon=True)
    proxy_thread.start()

    # Open card context
    cardContext = gp.OPGP_CARD_CONTEXT()
    cardContext.libraryName = "gpsocketconnectionplugin"
    cardContext.libraryVersion = "1"
    gp.OPGP_establish_context(cardContext)

    # Connect to a card and print the ATR
    cardInfo = gp.OPGP_CARD_INFO()
    gp.OPGP_card_connect(cardContext, str(plugin_socket.fileno()), cardInfo, (gp.OPGP_CARD_PROTOCOL_T0 | gp.OPGP_CARD_PROTOCOL_T1))

    # List installed applets
    try:
        list_applets(cardContext, cardInfo)
    except gp.OPGPError as e:
        print(f"Failed: {e.errorMessage}")
        
    # Close connections
    gp.OPGP_card_disconnect(cardContext, cardInfo)
    gp.OPGP_release_context(cardContext)

    # Exit proxy
    stop_event.set()
    proxy_thread.join()

    # Close sockets
    plugin_socket.close()
    app_socket.close()

    # Close card connection
    card.connection.disconnect()
