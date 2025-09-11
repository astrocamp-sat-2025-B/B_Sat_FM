import socket
import time

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
SERVER_IP = "192.168.179.43"

# サーバーのポート番号
SERVER_PORT = 4242

# 画像関連の設定 ('g'コマンド用)
# YUYVフォーマットで横320ピクセルなので、320ピクセル * 2バイト/ピクセル = 640 バイト
LINE_WIDTH_BYTES = 640
# --- 設定ここまで ---


def run_sequence_client():
    """
    1回の接続で、'g'コマンドによる画像データ取得と、
    'p'コマンドによるセンサーデータ取得を連続して実行するクライアント。
    """
    print(f"サーバー {SERVER_IP}:{SERVER_PORT} への接続を試みます...")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_IP, SERVER_PORT))
            print("サーバーに接続しました。")

            # --- 1. 'g'コマンドで画像データ(1行分)を取得 ---
            print("\n--- ステップ1: 画像データ(1行分)の取得 ---")
            g_command = "g"
            print(f"画像取得コマンドを送信: '{g_command}'")
            s.sendall(g_command.encode("utf-8"))

            print(f"{LINE_WIDTH_BYTES} バイトの画像データを受信します...")
            chunks = []
            bytes_received = 0
            while bytes_received < LINE_WIDTH_BYTES:
                chunk = s.recv(min(LINE_WIDTH_BYTES - bytes_received, 4096))
                if not chunk:
                    raise RuntimeError("画像受信中にサーバーとの接続が切れました")
                chunks.append(chunk)
                bytes_received += len(chunk)
                print(
                    f"\r画像受信済み: {bytes_received} / {LINE_WIDTH_BYTES} バイト",
                    end="",
                )

            print("\n画像データの受信完了。")
            image_data = b"".join(chunks)
            hex_representation = image_data.hex(" ")
            print(f"  受信データ (先頭32バイト): {hex_representation[:32*3]}...")

            # サーバー側が次のコマンドを受け付けるための短い待機
            time.sleep(0.1)

            # --- 2. 'p'コマンドでセンサーデータを取得 ---
            print("\n--- ステップ2: センサーデータの取得 ---")
            p_command = "p"
            print(f"センサー取得コマンドを送信: '{p_command}'")
            s.sendall(p_command.encode("utf-8"))

            response_data = s.recv(64)
            if not response_data:
                print("センサーデータ受信: サーバーから応答がありませんでした。")
            else:
                degree_str = response_data.decode("utf-8").strip()
                print(f"サーバーからの応答: {degree_str}")
                try:
                    degree_val = float(degree_str)
                    print(f"太陽光センサーの角度: {degree_val:.2f} 度")
                except ValueError:
                    print(
                        "エラー: 受信したセンサーデータを数値に変換できませんでした。"
                    )

            print("\n全ての処理が完了しました。接続を終了します。")

    except ConnectionRefusedError:
        print(
            "エラー: 接続が拒否されました。サーバーが起動しているか、IPアドレスとポートが正しいか確認してください。"
        )
    except socket.gaierror:
        print("エラー: IPアドレスの形式が正しくないか、解決できませんでした。")
    except Exception as e:
        print(f"予期せぬエラーが発生しました: {e}")


if __name__ == "__main__":
    run_sequence_client()
