import socket

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
SERVER_IP = "192.168.179.43"

# サーバーのポート番号
SERVER_PORT = 4242

# 送信するコマンド ('g' = get image data)
PAYLOAD = "g"

# 受信する画像の1行のバイト数 (camera.h の FRAME_WIDTH と一致させる)
# YUYVフォーマットで横320ピクセルなので、320ピクセル * 2バイト/ピクセル = 640 バイト
LINE_WIDTH_BYTES = 640
# --- 設定ここまで ---


def run_tcp_client():
    """
    サーバーに画像取得コマンドを送信し、1行分の画像データをバイナリで受信するクライアント。
    """
    print(f"サーバー {SERVER_IP}:{SERVER_PORT} への接続を試みます...")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            # サーバーに接続
            s.connect((SERVER_IP, SERVER_PORT))
            print("サーバーに接続しました。")

            # 画像取得コマンドをutf-8形式のバイト列にエンコードして送信
            print(f"画像取得コマンドを送信: '{PAYLOAD}'")
            s.sendall(PAYLOAD.encode("utf-8"))

            # --- バイナリデータ受信処理 ---
            print(f"{LINE_WIDTH_BYTES} バイトの画像データを受信します...")

            chunks = []
            bytes_received = 0
            # 期待するバイト数を受信するまでループで待機
            while bytes_received < LINE_WIDTH_BYTES:
                # 一度に受信する最大サイズを指定 (2048など大きめの値でOK)
                chunk = s.recv(min(LINE_WIDTH_BYTES - bytes_received, 2048))
                if not chunk:
                    # サーバーがデータを送り切る前に接続を閉じた場合
                    raise RuntimeError("サーバーとの接続が予期せず切れました")

                chunks.append(chunk)
                bytes_received += len(chunk)
                print(f"受信済み: {bytes_received} / {LINE_WIDTH_BYTES} バイト")

            # 受信したチャンク（断片）を結合して1つのバイト列にする
            image_data = b"".join(chunks)

            print("\n受信完了。")
            print(f"受信データ長: {len(image_data)} バイト")

            # 受信したデータを16進数で表示 (長すぎるので先頭と末尾の一部のみ)
            hex_representation = image_data.hex(" ")
            print(f"受信データ (16進数, 先頭32バイト): {hex_representation[:32*3]}...")
            print(f"受信データ (16進数, 末尾32バイト): ...{hex_representation[-32*3:]}")

            print("\n接続を終了します。")

    except ConnectionRefusedError:
        print(
            "エラー: 接続が拒否されました。サーバーが起動しているか、IPアドレスとポートが正しいか確認してください。"
        )
    except socket.gaierror:
        print("エラー: IPアドレスの形式が正しくないか、解決できませんでした。")
    except Exception as e:
        print(f"予期せぬエラーが発生しました: {e}")


if __name__ == "__main__":
    run_tcp_client()
