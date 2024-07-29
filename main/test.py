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
imu_data = [0.0 for _ in range(9)]
init_data1 = [[0.0 for _ in range(5)] for _ in range(5)]
init_data2 = [0.0 for _ in range(5)]
diff_data1 = [[0.0 for _ in range(5)] for _ in range(5)]
diff_data2 = [0.0 for _ in range(5)]
radius = [0.0 for _ in range(5)]
vertor_x = [[0.0 for _ in range(5)] for _ in range(5)]
vertor_y = [[0.0 for _ in range(5)] for _ in range(5)]
point_x = [[0.0 for _ in range(5)] for _ in range(5)]
point_y = [[0.0 for _ in range(5)] for _ in range(5)]

r = 0  # 变量 r，用于存储输入的数字

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
    # calculate_total_force()
    a=3.328
    b=0
    # data1=np.array([[a,a,a,a,a],[a,a,a,a,a],[a,a,a,a,a],[a,a,a,a,a],[a,a,a,a,a]])
    # data2=np.array([[1.75],[1.582],[1.77],[1.816],[b]])

def transfer(voltage, i, j):
    closest_kg = None
    min_diff = float('inf')  # 初始化最小差值为无穷大
    file_name = f"sensor[{i}][{j}].txt"  # 根据传入的i和j生成文件名

    try:
        with open(file_name, "r") as file:
            for line in file:
                match = re.match(r'kg:\s*([\d.]+),\s*value:\s*([\d.]+)', line)
                if match:
                    kg = float(match.group(1))
                    value = float(match.group(2))
                    diff = abs(voltage - value)  # 计算电压与记录值之间的差距
                    if diff < min_diff:
                        min_diff = diff
                        closest_kg = kg
        if voltage <= 1 or voltage >= 3:
            return 0.0  # 如果电压小于或等于1，直接返回0.0
        if closest_kg is not None:
            return closest_kg  # 返回最接近的重量
        else:
            print("Matching kg not found for the given voltage.")
            return None
    except FileNotFoundError:
        print(f"{file_name} not found.")
        return None

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
    # print("Differences calculated.")

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

def get_x_point(j, rol, radius):
    if radius == 0:
        return 0
    if radius == 10:
        return 0
    theta = 2.5*180 / (math.pi * radius)
    i = j 
    return math.sin(i * theta)

def get_y_point(j, rol, radius):
    if radius == 0:
        return 1
    if radius >6:
        return 1
    theta = 2.5*180 / (math.pi * radius)
    i = j 
    return math.cos(i * theta)

def calculate_forces(radius):
    global force_x, force_y
    force_x = 0
    force_y = 0
    
    k, l = 4, 4
    if(radius<6):
        force_y += transfer(data1[k][l],k,l) 
        for i in range(5):
            force_y += transfer(data1[4][i],4,i) 
        i=0
        for i in range(4):
            force_y += transfer(data1[i][0],i,0)
            j=1
            for j in range(4):
                force_x +=  - (point_x[i][j+1] * transfer(data1[i][j+1],i,j+1))
                force_y += (point_y[i][j+1] * transfer(data1[i][j+1],i,j+1))
                # print(f"pointy_i:{i}j:{j+1}:{point_y[4][0]}")
                # print(f"f_y:4j:0:{transfer(data1[4][0])}")
    elif(radius>=6):
        force_x=0
        for i in range(5):
            for j in range(5):
                force_y +=   transfer(data1[i][j],i,j)
    print(f"{transfer(data1[0][0],0,0)}")
    print(f"{transfer(data1[0][1],0,1)}")

    print(f"x{force_x},,y{force_y}")

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

    ax4.clear()
    ax4.axis('off')
    imu_text = "\n".join([f"IMU{i+1}: {imu_data[i]:.3f}" for i in range(9)])
    ax4.text(0.5, 0.5, imu_text, fontsize=12, ha='center', va='center', transform=ax4.transAxes)

    ax5.clear()
    ax5.set_xlim(-5, 5)
    ax5.set_ylim(-5, 5)
    ax5.set_xlabel('X Axis')
    ax5.set_ylabel('Y Axis')
    ax5.set_title('Vector Points Visualization')

    # 获取最接近的 r 值并显示在图形界面上
    closest_r_values = get_closest_r_value(data2)
    for col, r in closest_r_values:
        if r is not None:
            ax1.text(col, 0, f'r: {r}', color='red', fontsize=12, ha='center')
            radius[col] = r

    # 计算并显示向量点
    for rol in range(5):
        if radius[rol] != 0:
            for j in range(5):
                point_x[rol][j] = get_x_point(j, rol, radius[rol])
                point_y[rol][j] = get_y_point(j, rol, radius[rol])
                ax5.plot(point_x[rol][j], point_y[rol][j], 'bo')
                ax5.text(point_x[rol][j], point_y[rol][j], f'Point {rol}-{j}\n({point_x[rol][j]:.2f}, {point_y[rol][j]:.2f})', fontsize=8)

    # 计算并显示力
    calculate_forces(radius[1])
    ax1.text(0.5, 0.1, f'Force X: {force_x:.2f}', transform=ax1.transAxes, fontsize=12, color='red')
    ax1.text(0.5, 0.05, f'Force Y: {force_y:.2f}', transform=ax1.transAxes, fontsize=12, color='red')

    return scatter1

def update_sensor_file(kg, value, file_name):
    print(f"Attempting to update file: {file_name} with kg: {kg}, value: {value}")  # Debug print
    try:
        with open(file_name, "r") as file:
            lines = file.readlines()

        found = False
        with open(file_name, "w") as file:
            for line in lines:
                if line.startswith(f"kg: {kg}"):
                    file.write(f"kg: {kg}, value: {value}\n")
                    found = True
                    print(f"Updated line for kg: {kg}")  # Debug print
                else:
                    file.write(line)

            if not found:
                file.write(f"kg: {kg}, value: {value}\n")
                print(f"Added new line for kg: {kg}")  # Debug print

    except FileNotFoundError:
        with open(file_name, "w") as file:
            file.write(f"kg: {kg}, value: {value}\n")
        print(f"File {file_name} not found. Created new file and added kg.")  # Debug print


def submit(text):
    global r
    global data2
    # 处理 init kg[i][j] kg_value 命令
    match = re.match(r'init kg\[(\d+)\]\[(\d+)\] ([\d.]+)', text)
    if match:
        i = int(match.group(1))
        j = int(match.group(2))
        kg_value = float(match.group(3))

        if i < 0 or i >= len(data1) or j < 0 or j >= len(data1[0]):
            print("Index out of range.")
            return

        file_name = f"sensor[{i}][{j}].txt"
        update_sensor_file(kg_value, data1[i][j], file_name)
        print(f"Updated {file_name} with kg: {kg_value}, value: {data1[i][j]}")
    else:
        # 处理 init [column index] [r value] 命令
        match = re.match(r'init\s*(\d+)\s*([\d.]+)', text)
        if match:
            col = int(match.group(1))
            r = float(match.group(2))

            if col < 0 or col >= len(data2):
                print(f"Column index out of range. It should be between 0 and {len(data2) - 1}.")
                return

            lines = []
            try:
                with open("output.txt", "r") as file:
                    lines = file.readlines()
            except FileNotFoundError:
                pass

            new_line = f"col: {col}, r: {r}, value: {data2[col]}\n"
            found = False
            for i in range(len(lines)):
                match = re.match(r'col:\s*(\d+),\s*r:\s*([\d.]+),\s*value:\s*([\d.]+)', lines[i])
                if match and int(match.group(1)) == col:
                    lines[i] = new_line
                    found = True
                    break

            if not found:
                lines.append(new_line)

            with open("output.txt", "w") as file:
                file.writelines(lines)

            print(f"Variable r set to {r}, col: {col}, value: {data2[col]} and updated in output.txt")

        elif text == "save":
            save_initial_data()

        elif text == "clear":
            open("output.txt", "w").close()
            print("output.txt has been cleared.")
        else:
            print("Invalid command. Use: init kg[i][j] value, init [col] [r], save, or clear")


fig, ((ax1, ax4), (ax5, ax5)) = plt.subplots(2, 2, figsize=(15, 15))
plt.subplots_adjust(bottom=0.2)
ani = animation.FuncAnimation(fig, update_plot, interval=1000, cache_frame_data=False)

axbox = plt.axes([0.3, 0.05, 0.4, 0.075])
text_box = TextBox(axbox, 'Command: ')
text_box.on_submit(submit)

ser = serial.Serial('COM5', 115200)

def read_serial():
    count = 0
    start_time = time.time()
    global data1
    global data2
    while True:
        data = ser.readline().decode('utf-8', errors='ignore').strip()
        count += 1
        global radius
        process_data(data)

        current_time = time.time()
        if current_time - start_time >= 1.0:
            frequency = count / (current_time - start_time)
            count = 0
            start_time = current_time

            for i in range(5):
                for j in range(5):
                    point_x[i][j] = get_x_point(j, i, radius[i])

                    point_y[i][j] = get_y_point(j, i, radius[i])

thread = threading.Thread(target=read_serial)
thread.daemon = True
thread.start()

plt.show()
