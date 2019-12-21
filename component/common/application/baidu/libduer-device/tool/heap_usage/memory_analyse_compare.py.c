import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator, FormatStrFormatter

split_show_threshold=114000
#memory_collect_info_files = ["hm_collect_info.txt", "hm_collect_info_2.txt"]
memory_collect_info_files = ["hm_collect_info_0_2voice.txt", "hm_collect_info_3_2voice.txt"]

def get_line_color(number):
    colors = ['black','red','green','cyan','blue','fuchsia','gray','maroon','peru','gold','chartreuse', 'lightseagreen','pink','purple']
    index = number % len(colors)
    return colors[index]

def concise_name(name):
    index_of_ul = name.find('_')
    if (index_of_ul and index_of_ul < len(name)):
        return name[index_of_ul+1:]
    else:
        return name

num_of_files = len(memory_collect_info_files)
start_plot_index = 1
plt.figure(figsize=(100,30))

for memory_collect_info_file in memory_collect_info_files:
    df = pd.read_csv(memory_collect_info_file,header=0, index_col=0)

    col_num= len(df.columns)
    print("col_num:", col_num)
    print(df.columns)
    print(df.columns[col_num-1])
    values = df.get_values()
    index_col_value=df.index.values
    last_col_value=values[:, col_num-1]
    print(values)
    print(last_col_value)
    print(index_col_value)
    #plt.plot(index_col_value, last_col_value)
    print("========================================")

    #sub_plot_hm = [(0,-1), (0,2300), (2300,7000), (7000,13500), (13500,17100), (17100, 19700), (19700, 23300), (23300,26000)]
    sub_plot_hm = [(0,30000)]
    sub_plot_hm_index = 0
    xmajorlocator = MultipleLocator(200)
    xminorLocator = MultipleLocator(100)
    ymajorlocator = MultipleLocator(2000)
    yminorLocator = MultipleLocator(1000)
    #ymajorlocator = MultipleLocator(200)
    #yminorLocator = MultipleLocator(100)

    #sub_plot_hm = [(2800,3800)]
    #sub_plot_hm_index = 100
    #xmajorlocator = MultipleLocator(20)
    #xminorLocator = MultipleLocator(10)
    label_box = dict(facecolor='yellow', pad=3, alpha=0.2)
    title_box = dict(facecolor='blue', pad=3, alpha=0.2)

    for index_start, index_end in sub_plot_hm:
        ax = plt.subplot(num_of_files, 1, start_plot_index)
        ax.set_title("libduer-device heap memory usage", bbox=title_box)
        ax.set_ylabel("heap memory used(bytes)", bbox=label_box)
        ax.set_xlabel("heap memory update index", bbox=label_box)

        start_plot_index += 1
        ax.xaxis.set_major_locator(xmajorlocator)
        ax.yaxis.set_major_locator(ymajorlocator)
        ax.xaxis.set_minor_locator(xminorLocator)
        ax.yaxis.set_minor_locator(yminorLocator)
        plt.xticks(rotation='vertical')
        plt.grid()

        show_line_labels = []
        show_lines = []
        for i in range(col_num):
            column_value = values[index_start:index_end, i]
            if column_value.any():
                cl_name = concise_name(df.columns[i])
                print("column_name:", cl_name, ";max:", column_value.max())
                #print(type(column_value))
                print(column_value)
                if column_value.max() < split_show_threshold:
                    line, = plt.plot(index_col_value[index_start:index_end], column_value, color=get_line_color(i), label=cl_name, linestyle='solid')#, marker='o')
                    show_line_labels.append(cl_name)
                    show_lines.append(line)

        plt.legend(handles=show_lines, labels=show_line_labels, loc='best');
        file_name = "test" + str(sub_plot_hm_index) + ".svg"
    #plt.savefig(file_name, format="svg")
        sub_plot_hm_index += 1
    #    plt.cla()
    print("========================================")

plt.show()

