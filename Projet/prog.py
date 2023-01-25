import tensorflow as tf
import numpy as np
from tensorflow import keras
import matplotlib.pyplot as plt
import matplotlib.image as img


def main():
    # print("TensorFlow version:", tf.__version__)
    ## 1 ##
    image = img.imread("./data/cartoon.png")
    plt.figure(figsize= (10, 10))
    plt.imshow(image)
    plt.show()
    green_channel = image[..., 1]
    plt.imshow(green_channel, cmap="gray")
    plt.show()


if __name__ == "__main__":
    main()

