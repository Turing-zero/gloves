for col in range(5):
    for row in range(5):
        # 创建并打开每个sensor对应的文件
        file_name = f"sensor[{col}][{row}].txt"
        with open(file_name, "w") as file:  # 使用"w"模式，每次运行代码时覆盖旧文件
            for j in range(100):
                r = j * 0.1
                # 使用格式化方法保留一位小数
                file.write(f"kg: {r:.1f}, value: 0\n")
