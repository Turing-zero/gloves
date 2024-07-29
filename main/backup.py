import time
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import TextBox
import math
import re
import threading

# 初始化data1和data2数组
data1 = [[0.0 for _ in range(5)] for _ in range(5)]
data2 = [0.0 for _ in range(5)]
imu_data =[0.0 for _ in range(9)]
init_data1 = [[0.0 for _ in range(5)] for _ in range(5)]
init_data2 = [0.0 for _ in range(5)]
diff_data1 = [[0.0 for _ in range(5)] for _ in range(5)]
diff_data2 = [0.0 for _ in range(5)]
radius = [0.0 for _ in range(5)]
vertor_x = [[0.0 for _ in range(5)] for _ in range(5)]
vertor_y = [[0.0 for _ in range(5)] for _ in range(5)]
point_x =[0.0 for _ in range(5)]
point_y =[0.0 for _ in range(5)]

r = 0  # 变量 r，用于存储输入的数字

class ForceVector:
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return f"Total Force Vector:\nX: {self.x:.3f}\nY: {self.y:.3f}\nZ: {self.z:.3f}"

total_force = ForceVector()

def process_data(line):
    global data1, data2, imu_data, diff_data1, diff_data2
    if line.startswith("Data1"):
        parts = line.split(":")
        row = int(parts[1])
        values = parts[2].split(",")
        for col in range(5):
            data1[row][col] = float(values[col])
    elif line.startswith("Data2"):
        parts = line.split(":")
        values = parts[1].split(",")
        for i in range(5):
            data2[i] = float(values[i])
    elif line.startswith("IMU"):
        parts = line.split(":")
        values = parts[1].split(",")
        for i in range(9):
            imu_data[i] = float(values[i].strip())
    calculate_differences()
    #calculate_total_force()

def save_initial_data():
    global init_data1, init_data2, data1, data2
    for i in range(5):
        for j in range(5):
            init_data1[i][j] = data1[i][j]
    for i in range(5):
        init_data2[i] = data2[i]
    print("Initial data saved.")

def calculate_differences():
    global data1, data2, init_data1, init_data2, diff_data1, diff_data2
    for i in range(5):
        for j in range(5):
            diff_data1[i][j] = data1[i][j] - init_data1[i][j]
    for i in range(5):
        diff_data2[i] = data2[i] - init_data2[i]
    #print("Differences calculated.")

def calculate_total_force():
    global total_force, diff_data1, diff_data2
    total_force = ForceVector()
    is_hand_open = all(abs(diff) <= 0.3 for diff in diff_data2)

    if is_hand_open:
        # Hand is open
        for i in range(5):
            for j in range(5):
                total_force.z += diff_data1[i][j]
    else:
        # Hand is closed
        for j in range(5):
            for i in range(3):
                if j == 0:
                    total_force.x += diff_data1[i][j] - diff_data1[0][j]
                elif j == 4:
                    total_force.x += diff_data1[i][j] - diff_data1[i][4]
    print(total_force)

def read_output_file():
    data = []
    try:
        with open("output.txt", "r") as file:
            for line in file:
                match = re.match(r'col:\s*(\d+),\s*r:\s*([\d.]+),\s*value:\s*([\d.]+)', line)
                if match:
                    r_value = float(match.group(2))
                    col_value = int(match.group(1))
                    data_value = float(match.group(3))
                    data.append((r_value, col_value, data_value))
    except FileNotFoundError:
        print("output.txt not found.")
    return data

def get_closest_r_value(data2):
    closest_r_values = []

    # 读取 output.txt 文件
    output_data = read_output_file()

    # 遍历 data2 数组中的每个值
    for col_index, data2_value in enumerate(data2):
        closest_r = None
        min_diff = float('inf')
        
        # 查找 output.txt 中与 data2[col_index] 最接近的值
        for r_value, col_value, data_value in output_data:
            if col_value == col_index:
                diff = abs(data2_value - data_value)
                if diff < min_diff:
                    min_diff = diff
                    closest_r = r_value
        
        closest_r_values.append((col_index, closest_r))
    
    return closest_r_values

def update_plot(frame):
    ax1.clear()
    ax1.set_xlim(-0.5, 4.5)
    ax1.set_ylim(-0.5, 4.5)
    ax1.set_xlabel('Column Index')
    ax1.set_ylabel('Row Index')
    ax1.set_title('Pressure Differences Visualization')

    # Flatten the diff_data1 array
    diff_data1_flat = np.array(diff_data1).flatten()
    x, y = np.meshgrid(range(5), range(5))
    x = x.flatten()
    y = y.flatten()

    # Create scatter plot for diff_data1
    scatter1 = ax1.scatter(x, y, c=diff_data1_flat, cmap='viridis', s=100, edgecolor='k', vmin=-3.3, vmax=3.3)
    if not hasattr(update_plot, "colorbar1"):
        update_plot.colorbar1 = plt.colorbar(scatter1, ax=ax1, label='Pressure Difference (diff_data1)')
    else:
        update_plot.colorbar1.update_normal(scatter1)

    ax2.clear()
    ax2.set_xlim(-0.5, 4.5)
    ax2.set_ylim(-1.0, 1.0)
    ax2.set_xlabel('Finger Index')
    ax2.set_ylabel('Bend Difference')
    ax2.set_title('Bend Differences Visualization')

    # Plot diff_data2 as bar chart
    x2 = np.arange(5)
    bars = ax2.bar(x2, diff_data2, color='blue', alpha=0.7)
    ax2.set_xticks(x2)
    ax2.set_xticklabels(['Finger1', 'Finger2', 'Finger3', 'Finger4', 'Finger5'])

    ax3.clear()
    ax3.set_xlim(-0.5, 2.5)
    ax3.set_ylim(-10, 10)
    ax3.set_xticks([0, 1, 2])
    ax3.set_xticklabels(['Force X', 'Force Y', 'Force Z'])
    ax3.set_title('Total Force Visualization')

    # Plot total force as bar chart
    forces = [total_force.x, total_force.y, total_force.z]
    bars3 = ax3.bar([0, 1, 2], forces, color='red', alpha=0.7)
    global radius
    ax4.clear()
    ax4.axis('off')
    imu_text = "\n".join([f"IMU{i+1}: {imu_data[i]:.3f}" for i in range(9)])
    ax4.text(0.5, 0.5, imu_text, fontsize=12, ha='center', va='center', transform=ax4.transAxes)

    # 获取最接近的 r 值并显示在图形界面上
    closest_r_values = get_closest_r_value(data2)
    for col, r in closest_r_values:
        
        if r is not None:
            ax1.text(col, r, f'r: {r}', color='red', fontsize=12, ha='center')
            radius[col]=r
    return scatter1, bars, bars3

def get_x_point(j,rol, radius):
    theta = 1 / (math.pi * radius * 180)
    i = j + 1
    vertor_x[rol][j] = radius * math.sin(i * theta)
    return vertor_x

def get_y_point(j,rol, radius):
    theta = 1 / (math.pi * radius * 180)
    i = j + 1
    vertor_y[rol][j] = radius * math.cos(i * theta)
    return vertor_y

def submit(text):
    global r
    global data2

    if text.startswith("init"):
        try:
            # 使用正则表达式解析输入
            match = re.match(r'init\s*(\d+)\s*([\d.]+)', text)
            if not match:
                raise ValueError("Please provide both col and r values in the format 'init <col> <r>'")

            col = int(match.group(1))
            r = float(match.group(2))

            if col < 0 or col >= len(data2):
                raise ValueError(f"Column index out of range. It should be between 0 and {len(data2) - 1}.")

            # 读取现有的文件内容
            lines = []
            try:
                with open("output.txt", "r") as file:
                    lines = file.readlines()
            except FileNotFoundError:
                pass

            # 查找需要修改的行
            new_line = f"col: {col}, r: {r}, value: {data2[col]}\n"
            found = False
            for i in range(len(lines)):
                match = re.match(r'col:\s*(\d+),\s*r:\s*([\d.]+),\s*value:\s*([\d.]+)', lines[i])
                if match and int(match.group(1)) == col and float(match.group(2)) == r:
                    lines[i] = new_line
                    found = True
                    break

            # 如果没有找到对应的行，则添加新的行
            if not found:
                lines.append(new_line)

            # 写回文件
            with open("output.txt", "w") as file:
                file.writelines(lines)

            print(f"Variable r set to {r}, col: {col}, value: {data2[col]} and updated in output.txt")

        except (IndexError, ValueError) as e:
            print(e)

    elif text == "save":
        save_initial_data()

    elif text == "clear":
        # 清空文件内容
        open("output.txt", "w").close()
        print("output.txt has been cleared.")

fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(12, 10))
plt.subplots_adjust(bottom=0.2)
ani = animation.FuncAnimation(fig, update_plot, interval=1000, cache_frame_data=False)

# 添加文本框
axbox = plt.axes([0.3, 0.05, 0.4, 0.075])
text_box = TextBox(axbox, 'Command: ')
text_box.on_submit(submit)

# 初始化串口
ser = serial.Serial('COM5', 115200)

def read_serial():
    count = 0  # 初始化数据计数器
    start_time = time.time()  # 记录起始时间

    while True:
        data = ser.readline().decode('utf-8', errors='ignore').strip()
        count += 1  # 对每一次数据读取进行计数
        
        global radius
        process_data(data)

        # 每秒计算一次频率并重置计数器
        current_time = time.time()
        if current_time - start_time >= 1.0:
            frequency = count / (current_time - start_time)  # 计算频率
            print(f"Frequency: {frequency} messages per second")  # 打印频率
            count = 0  # 重置计数器
            start_time = current_time  # 更新开始时间
            
            # 打印存储的数组
            print("Diff Data1:")
            for row in diff_data1:
                print(row)
            print("Diff Data2:")
            print(diff_data2)
            print("IMU Data:")
            print(imu_data)
            for i in range (5):
                for j in range(4):
                    point_x[i][j]=get_x_point(j,i,radius[i])
                    point_y[i][j]=get_y_point(j,i,radius[i])
            print(point_x[2][2])

# 启动串口读取线程
thread = threading.Thread(target=read_serial)
thread.daemon = True
thread.start()

plt.show()