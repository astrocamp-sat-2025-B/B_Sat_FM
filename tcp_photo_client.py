import socket

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
# SERVER_IP = "192.168.179.43"
SERVER_IP = "192.168.137.184"

# サーバーのポート番号
SERVER_PORT = 4242

# 画像関連の設定 ('g'コマンド用)
# YUYVフォーマットで横320ピクセルなので、320ピクセル * 2バイト/ピクセル = 640 バイト
LINE_WIDTH_BYTES = 640
# --- 設定ここまで ---


def handle_g_command(s):
    """'g'コマンドを送信し、画像データを受信する"""
    print(f"-> 'g'コマンドを送信...")
    s.sendall(b"g")

    print(f"<- {LINE_WIDTH_BYTES} バイトの画像データを受信します...")
    chunks = []
    bytes_received = 0
    while bytes_received < LINE_WIDTH_BYTES:
        chunk = s.recv(min(LINE_WIDTH_BYTES - bytes_received, 4096))
        if not chunk:
            raise RuntimeError("画像受信中にサーバーとの接続が切れました")
        chunks.append(chunk)
        bytes_received += len(chunk)

    image_data = b"".join(chunks)
    print("<- 画像データの受信完了。")
    hex_representation = image_data.hex(" ")
    print(f"   受信データ (先頭32バイト): {hex_representation[:32*3]}...")


def handle_p_command(s):
    """'p'コマンドを送信し、センサーデータを受信する"""
    print(f"-> 'p'コマンドを送信...")
    s.sendall(b"p")

    response_data = s.recv(64)
    if not response_data:
        print("<- センサーデータ受信: サーバーから応答がありませんでした。")
    else:
        degree_str = response_data.decode("utf-8").strip()
        print(f"<- サーバーからの応答: {degree_str}")
        try:
            degree_val = float(degree_str)
            print(f"   太陽光センサーの角度: {degree_val:.2f} 度")
        except ValueError:
            print("   エラー: 受信したセンサーデータを数値に変換できませんでした。")


def run_interactive_client():
    """
    サーバーに接続し、ユーザーの入力に応じてコマンドを繰り返し送信する
    対話的なクライアント。
    """
    print(f"サーバー {SERVER_IP}:{SERVER_PORT} への接続を試みます...")

    try:
        # サーバーに接続
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((SERVER_IP, SERVER_PORT))
        print("サーバーに接続しました。")
        print("-" * 40)

        # ユーザーが'exit'を入力するまでループ
        while True:
            print("\nコマンドを入力してください ('g', 'p', または 'exit'):")
            command = input("> ").lower().strip()

            if command == "g":
                handle_g_command(s)
            elif command == "p":
                handle_p_command(s)
            elif command == "exit":
                print("クライアントを終了します。")
                break
            else:
                print(f"無効なコマンドです: '{command}'")

    except ConnectionRefusedError:
        print(
            "エラー: 接続が拒否されました。サーバーが起動しているか、IPアドレスとポートが正しいか確認してください。"
        )
    except socket.gaierror:
        print("エラー: IPアドレスの形式が正しくないか、解決できませんでした。")
    except Exception as e:
        print(f"予期せぬエラーが発生しました: {e}")
    finally:
        # ソケットを閉じる
        if "s" in locals() and s.fileno() != -1:
            s.close()
            print("接続を終了しました。")


if __name__ == "__main__":
    run_interactive_client()
