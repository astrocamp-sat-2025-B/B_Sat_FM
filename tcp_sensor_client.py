import socket

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
SERVER_IP = "192.168.179.43"

# サーバーのポート番号
SERVER_PORT = 4242

# ★ 送信するコマンドを 'p' に変更
PAYLOAD = "p"
# --- 設定ここまで ---


def get_light_degree():
    """
    サーバーに 'p' コマンドを送信し、light_deg の値を受信して表示する。
    """
    print(f"サーバー {SERVER_IP}:{SERVER_PORT} への接続を試みます...")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            # サーバーに接続
            s.connect((SERVER_IP, SERVER_PORT))
            print("サーバーに接続しました。")

            # コマンドを送信
            print(f"コマンドを送信: '{PAYLOAD}'")
            s.sendall(PAYLOAD.encode("utf-8"))

            # サーバーからの応答を受信 (最大64バイトもあれば十分)
            response_data = s.recv(64)
            if not response_data:
                print("サーバーから応答がありませんでした。")
                return

            # 受信したバイト列を文字列にデコード
            degree_str = response_data.decode("utf-8").strip()
            print(f"サーバーからの応答: {degree_str}")

            # 受信した文字列をfloat型に変換してみる
            try:
                degree_val = float(degree_str)
                print(f"太陽光センサーの角度: {degree_val:.2f} 度")
            except ValueError:
                print("エラー: 受信したデータを数値に変換できませんでした。")

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
    get_light_degree()
