import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter


# helpers
def parse_thread_num(name):
    thread_args = [
        word for word in name.split('/') if word.startswith('threads')
    ]
    assert len(thread_args) == 1
    num = thread_args[0].split(':')[-1]
    return int(num)


def parse_queue_type(name):
    return name.split('/')[0].split('<')[1]


def rate_format(rate, pos):
    return '{:.1f} M/s'.format(rate * 1e-6)


# data processing
def preprocess(data):
    data.dropna(axis=1, inplace=True)

    data['QueueType'] = data['name'].apply(parse_queue_type)
    data['num_total_thread'] = data['name'].apply(parse_thread_num)
    data['num_thread'] = data['num_total_thread'] / 2
    data.drop('name', axis=1, inplace=True)

    units = data['time_unit'].unique()
    assert len(units) == 1 and units[0] == 'ns'

    data['elapsed_seconds'] = data['iterations'] * data['cpu_time'] / data[
        'num_thread'] / 10.0**9
    data['pop_rate'] = data['pop_count'] / data['elapsed_seconds']
    data['push_rate'] = data['push_count'] / data['elapsed_seconds']

    return data


def plot_pop_rate(data):
    plt.figure(dpi=1000)
    ax = sns.lineplot(x='num_thread', y='pop_rate', hue='QueueType', data=data)
    ax.yaxis.set_major_formatter(FuncFormatter(rate_format))
    plt.legend(bbox_to_anchor=(1, 1))
    plt.xlabel('Number of thread')
    plt.ylabel('Pop throughput (M/s)')
    plt.ylim(bottom=0, top=5500000)
    plt.savefig('pop_throughput', bbox_inches='tight')


def plot_push_rate(data):
    plt.figure(dpi=1000)
    ax = sns.lineplot(x='num_thread',
                      y='push_rate',
                      hue='QueueType',
                      data=data)
    ax.yaxis.set_major_formatter(FuncFormatter(rate_format))
    plt.legend(bbox_to_anchor=(1, 1))
    plt.xlabel('Number of thread')
    plt.ylabel('Push throughput (M/s)')
    plt.ylim(bottom=0, top=5500000)
    plt.savefig('push_throughput', bbox_inches='tight')


def main(path):
    data = pd.read_csv(path)
    data = preprocess(data)
    sns.set()
    plot_pop_rate(data)
    plot_push_rate(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('file', help='Benchmark CSV result')
    args = parser.parse_args()
    main(args.file)
