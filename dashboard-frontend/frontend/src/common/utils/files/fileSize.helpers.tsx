export function getReadablyFileSize(byteFileSize: number) {
    let tempSize = byteFileSize;
    const filePowers = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB'];

    let i = 0;
    while (tempSize > 900 && i < filePowers.length - 1) {
        tempSize /= 1000;
        i++;
    };

    const exactSize = (Math.round(tempSize * 100) / 100) + ' ' + filePowers[i];

    return exactSize
}
