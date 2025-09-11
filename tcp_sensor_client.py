import socket

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
SERVER_IP = "192.168.179.43"

# サーバーのポート番号
SERVER_PORT = 4242

# ★ 画像全体のサイズを定義 (camera.h の設定と一致させる)
IMAGE_WIDTH_BYTES = 320 * 2  # FRAME_WIDTH
IMAGE_HEIGHT = 240  # FRAME_HEIGHT
TOTAL_IMAGE_BYTES = IMAGE_WIDTH_BYTES * IMAGE_HEIGHT

# ★ 保存するRAWバイナリファイル名
OUTPUT_BINARY_FILENAME = "received_image.bin"
# --- 設定ここまで ---


def handle_g_command(s):
    """'g'コマンドを送信し、画像データ全体を受信してファイルに保存する"""
    print(f"-> 'g'コマンドを送信...")
    s.sendall(b"g")

    print(f"<- {TOTAL_IMAGE_BYTES} バイトの画像データを受信します...")

    chunks = []
    bytes_received = 0
    # ★ ループ条件を画像全体のサイズに変更
    while bytes_received < TOTAL_IMAGE_BYTES:
        # 一度に受信する最大サイズを指定 (4096など大きめの値でOK)
        chunk = s.recv(min(TOTAL_IMAGE_BYTES - bytes_received, 4096))
        if not chunk:
            raise RuntimeError("画像受信中にサーバーとの接続が予期せず切れました")

        chunks.append(chunk)
        bytes_received += len(chunk)
        # 進捗が分かりやすいように表示
        print(f"\r   受信済み: {bytes_received} / {TOTAL_IMAGE_BYTES} バイト", end="")

    print("\n<- 画像データの受信完了。")
    full_image_data = b"".join(chunks)

    # ★★★ 受信したバイナリデータ全てをファイルに保存する処理 ★★★
    print(f"   受信したバイナリデータを '{OUTPUT_BINARY_FILENAME}' に保存しています...")
    try:
        with open(OUTPUT_BINARY_FILENAME, "wb") as f:
            f.write(full_image_data)
        print(
            f"   バイナリデータの保存が完了しました。サイズ: {len(full_image_data)} バイト"
        )
    except Exception as e:
        print(f"   バイナリファイルの保存中にエラーが発生しました: {e}")


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
            print(
                "\nコマンドを入力してください ('g' for full image, 'p', または 'exit'):"
            )
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
