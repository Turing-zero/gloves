import time
import serial

with serial.Serial('COM3', 115200) as ser:
    count = 0  # 初始化数据计数器
    start_time = time.time()  # 记录起始时间

    while True:
        data = ser.readline()
        data = data.decode()
        count += 1  # 对每一次数据读取进行计数

        if data.startswith("IMU"):
            print("IMUDATA : ", data)
        elif data.startswith("Data1"):
            print("data1", data)
        elif data.startswith("Data2"):
            print("data2", data)
        elif data.startswith("time_diff"):
            print("time_diff:", data)
        else:
            print("type not valid")
        
        # 每秒计算一次频率并重置计数器
        current_time = time.time()
        if current_time - start_time >= 1.0:
            frequency = count / (current_time - start_time)  # 计算频率
            print(f"Frequency: {frequency} messages per second")  # 打印频率
            count = 0  # 重置计数器
            start_time = current_time  # 更新开始时间
