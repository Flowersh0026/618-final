from plot import *


def plot_opt_cmp(data, queue_type, column):
    plt.figure(dpi=1000)
    ax = sns.lineplot(x='num_thread',
                      y=column,
                      hue='Optimizations',
                      data=data[data['QueueType'] == queue_type])
    ax.yaxis.set_major_formatter(FuncFormatter(rate_format))
    plt.legend(bbox_to_anchor=(1, 1))
    plt.xlabel('Number of thread')
    plt.ylabel('Throughput (M/s)')
    plt.ylim(bottom=0, top=5500000)
    plt.savefig('opt_' + queue_type + '_' + column + '.png',
                bbox_inches='tight')


if __name__ == '__main__':
    baseline = pd.read_csv('no_optimization.csv')
    jemalloc = pd.read_csv('use_jemalloc.csv')
    aligned = pd.read_csv('align_cacheline.csv')
    full = pd.read_csv('use_jemalloc_and_align_cacheline.csv')

    baseline = preprocess(baseline)
    jemalloc = preprocess(jemalloc)
    aligned = preprocess(aligned)
    full = preprocess(full)

    baseline['Optimizations'] = 'None (baseline)'
    jemalloc['Optimizations'] = 'jemalloc'
    aligned['Optimizations'] = 'Aligned'
    full['Optimizations'] = 'jemalloc & Aligned'

    data = pd.concat([baseline, jemalloc, aligned, full])

    sns.set()
    plot_opt_cmp(data, 'RtmQueue', 'push_rate')
    plot_opt_cmp(data, 'RtmQueue', 'pop_rate')
    plot_opt_cmp(data, 'CasQueue', 'push_rate')
    plot_opt_cmp(data, 'CasQueue', 'pop_rate')
