import socket

# --- 設定項目 ---
# Pico WのIPアドレスに書き換えてください
# (IPアドレスはPico W起動時のシリアルモニター出力で確認できます)
SERVER_IP = "192.168.179.43"

# サーバーのポート番号 (tcp_sever.cppで定義されているもの)
SERVER_PORT = 4242  #

# 送信するメッセージ
PAYLOAD = "hello world"
# --- 設定ここまで ---


def run_tcp_client():
    """
    指定されたIPアドレスとポートに接続し、メッセージを送信して応答を受信するTCPクライアント。
    """
    print(f"サーバー {SERVER_IP}:{SERVER_PORT} への接続を試みます...")

    # withステートメントでソケットの自動クローズを保証
    # socket.AF_INET: IPv4を使用
    # socket.SOCK_STREAM: TCP通信を使用
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            # サーバーに接続
            s.connect((SERVER_IP, SERVER_PORT))
            print("サーバーに接続しました。")

            # メッセージをutf-8形式のバイト列にエンコードして送信
            print(f"メッセージを送信: {PAYLOAD}")
            s.sendall(PAYLOAD.encode("utf-8"))

            # サーバーからの応答を受信 (最大1024バイト)
            # Pico Wのサーバーは受信したデータをそのままエコーバックする
            response_data = s.recv(1024)

            # 受信したバイト列を文字列にデコードして表示
            print(f"サーバーからの応答: {response_data.decode('utf-8').strip()}")
            print("接続を終了します。")

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
