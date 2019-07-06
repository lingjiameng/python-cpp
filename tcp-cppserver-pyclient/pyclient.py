import time
import socket
import cv2
import numpy as np


class ImgReader(object):
    def __init__(self, addr):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        self.client.connect(addr)

    def get_img(self):

        t2 = time.time()

        self.client.send(b"GET IMG")  # 发送指令
        ##### 接收图片信息
        img_len = int(self.client.recv(1024))
        self.client.send(b"ACK")  # 发送ACK

        #接收剩余的图片信息
        img_full = b""
        while len(img_full) < img_len:
            img_full += self.client.recv(32768)

        ##字符串转数组
        img = np.fromstring(img_full, dtype=np.uint8, sep=",")
        img = cv2.imdecode(img, cv2.IMREAD_COLOR)
        print("RTT RECV Time: {:.5f} s".format(time.time()-t2))

        return img

    def close(self):
        self.client.close()


if __name__ == "__main__":
    img_reader = ImgReader(("localhost", 13456))

    for i in range(10):
        t0 = time.time()
        img = img_reader.get_img()
        print("Image Info: ",img.shape)

        cv2.imshow("test", img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()

    img_reader.close()
